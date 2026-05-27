#if 1
#include "App_IRTI.h"
#include "../../../ChappieBsp/Chappie.h"
#include "esp_heap_caps.h"
static std::string app_name = "IRTI";
static CHAPPIE* device;
#define _SCALE 9
#define MLX_H_Pixel 24
#define MLX_W_Pixel 32
#define Screen_H 280
#define Screen_W 240
#define MLX_MAX_Temp 300
#define MLX_MIN_Temp -20
#define abs(x)      ((x)>0?(x):-(x))
LV_IMG_DECLARE(ui_img_icon_IRTI_png);
//cfg define
static lv_timer_t* timer;
static float mlx90640To[MLX_W_Pixel * MLX_H_Pixel]={0};  // 从MLX90640读取的温度数据
static  TaskHandle_t Task_MLX = NULL;


bool MLX_isConnected=false,freeze=false,lock=false;
float T_max, T_min, T_avg; 
int max_x, max_y, min_x, min_y;
EmmaThermal::Thermal_Class::paramsMLX90640 mlx90640;

lv_obj_t * canvas;
// lv_color_t canvas_buf[LV_CANVAS_BUF_SIZE_TRUE_COLOR(Screen_W, Screen_H / 2)];
namespace App {
    


void create_canvas() {
    canvas = lv_canvas_create(lv_scr_act());
    // lv_canvas_set_buffer(canvas, canvas_buf, Screen_W,Screen_H, LV_IMG_CF_TRUE_COLOR);
    lv_obj_set_size(canvas, Screen_W,Screen_H);
    lv_obj_align(canvas, LV_ALIGN_CENTER, 0, 0);
}
//插值算法
float bilinear_interpolation(float x, float y, float *data, int width, int height) {
    int x1 = (int)x;
    int x2 = x1 + 1;
    int y1 = (int)y;
    int y2 = y1 + 1;

    if (x2 >= width) x2 = width - 1;
    if (y2 >= height) y2 = height - 1;

    float q11 = data[y1 * width + x1];
    float q12 = data[y2 * width + x1];
    float q21 = data[y1 * width + x2];
    float q22 = data[y2 * width + x2];

    float r1 = ((x2 - x) * q11 + (x - x1) * q21);
    float r2 = ((x2 - x) * q12 + (x - x1) * q22);
    float p = ((y2 - y) * r1 + (y - y1) * r2);

    return p;
}
// 伪彩色转换函数
// lv_color_t GrayToPseColor(float temp) {
//     // 假设温度范围的上下限为20到40摄氏度
//     uint8_t colorR, colorG, colorB;
//     // 将实际的温度数据转换为0~255之间的数值
//     uint8_t grayValue = (uint8_t)(255 * (temp - MLX_MIN_Temp) / (MLX_MAX_Temp - MLX_MIN_Temp));
//     // 使用转换后的数值代入伪彩编码计算函数，生成伪彩色
//     colorR = abs(255 - grayValue);
//     colorG = abs(127 - grayValue);
//     colorB = abs(0 - grayValue);
//     return lv_color_make(colorR, colorG, colorB);
// }
lv_color_t GrayToPseColor(float temperature) {
    // 假设温度范围在-20到300摄氏度之间
    float normalized = (temperature - MLX_MIN_Temp) / (MLX_MAX_Temp - MLX_MIN_Temp);
    if (normalized < 0) normalized = 0;
    if (normalized > 1) normalized = 1;

    // 线性插值计算颜色
    uint8_t r = (uint8_t)abs(255 * (1 - normalized));
    uint8_t g = (uint8_t)abs(127 * (1 - normalized));
    uint8_t b = (uint8_t)abs(255 * normalized);

    return lv_color_make(r, g, b);
}
void draw_image(float *mlx90640To) {
    for (int y = 0; y < Screen_H; y++) {
        for (int x = 0; x < Screen_W; x++) {
            float srcX = (float)x / Screen_W * (MLX_W_Pixel - 1);
            float srcY = (float)y / Screen_H * (MLX_H_Pixel - 1);
            float temp = bilinear_interpolation(srcX, srcY, mlx90640To, MLX_W_Pixel, MLX_H_Pixel);
            lv_color_t color = GrayToPseColor(temp);
            lv_canvas_set_px(canvas, x, y, color);
        }
    }
}
    /**
     * @brief Return the App name laucnher, which will be show on launcher App list
     * 
     * @return std::string 
     */
    std::string App_IRTI_appName()
    {
        return app_name;
    }


    /**
     * @brief Return the App Icon laucnher, NULL for default
     * 
     * @return void* 
     */
    void* App_IRTI_appIcon()
    {
        return (void*) &ui_img_icon_IRTI_png;
        // return NULL;
    }

    void Task_DrawBinMap(void * ptr){

    }

    /**
     * @brief Called when App is on create
     * 
     */
    static void Task_Update_MLX(void * ptr)
    {
        
        if (device->Thermal.isConnectd() == false){
            while (1){
                IRTI_LOG("[IRTI]:MLX90640 not detected. Please check wiring. Freezing.");
                vTaskDelay(1000);
            };
         }
         else{
            MLX_isConnected = true;
         }
        int status;
        static uint16_t eeMLX90640[832];
        status = device->Thermal.MLX90640_DumpEE(EMMA_Thermal_MLX_IIC_ADDR,eeMLX90640);
        if (status != 0)
         IRTI_LOG("[IRTI]:Failed to load system parameters");
         status = device->Thermal.MLX90640_ExtractParameters(eeMLX90640, &mlx90640);
        if (status != 0)
        {
            IRTI_LOG("Parameter extraction failed");
            IRTI_LOG(" status = %d",status);
        }
        //Once params are extracted, we can release eeMLX90640 array
        device->Thermal.begin();
        //Set MLX90640 FreshRate mode
        device->Thermal.MLX90640_SetRefreshRate(EMMA_Thermal_MLX_IIC_ADDR,device->Thermal.R_MOD5);
        static uint16_t mlx90640Frame[834];
        for(;MLX_isConnected==true;){
            lock = true;
            float vdd = device->Thermal.MLX90640_GetVdd(mlx90640Frame, &mlx90640);
            float Ta = device->Thermal.MLX90640_GetTa(mlx90640Frame, &mlx90640);
            float tr = Ta - MLX_TA_SHIFT; //Reflected temperature based on the sensor ambient temperature
            float emissivity = 0.95;
            device->Thermal.MLX90640_GetFrameData(EMMA_Thermal_MLX_IIC_ADDR, mlx90640Frame);
            device->Thermal.MLX90640_CalculateTo(mlx90640Frame, &mlx90640, emissivity, tr, mlx90640To);
            lock = false;
            T_min = mlx90640To[0];
            T_max = mlx90640To[0];
            T_avg = mlx90640To[0];
            for (int i = 1; i < 768; i++){ // 32*24 pixel
                if((mlx90640To[i] > -41) && (mlx90640To[i] < 301))
                    {
                        if(mlx90640To[i] < T_min){
                            T_min = mlx90640To[i];
                        }
                        if(mlx90640To[i] > T_max){
                            T_max = mlx90640To[i];
                            max_x = i / 32;
                            max_y = i % 32;
                        }
                    }
                else if(i > 0){   // temperature out of range
                        mlx90640To[i] = mlx90640To[i-1];
                }
                else{
                        mlx90640To[i] = mlx90640To[i+1];
                }
                T_avg = T_avg + mlx90640To[i];
            }
            vTaskDelay(5);
            draw_image(mlx90640To);
        }
        vTaskDelete(NULL); 
    }

    void App_IRTI_onCreate()
    {
        UI_LOG("[%s] onCreate\n", App_IRTI_appName().c_str());

        // init canvas
        create_canvas();
        xTaskCreate(Task_Update_MLX, "MLX_Update", 1024 * 6, NULL, 1, &Task_MLX);
    }
    

    /**
     * @brief Called repeatedly, end this function ASAP! or the App management will be affected
     * If the thing you want to do takes time, try create a taak or lvgl timer to handle them.
     * Try use millis() instead of delay() here
     * 
     */
    void App_IRTI_onLoop()
    {

    }


    /**
     * @brief Called when App is about to be destroy
     * Please remember to release the resourse like lvgl timers in this function
     * 
     */
    void App_IRTI_onDestroy()
    {
        UI_LOG("[%s] onDestroy\n", App_IRTI_appName().c_str());
        vTaskDelete(Task_MLX);
    }


    /**
     * @brief Launcher will pass the BSP pointer through this function before onCreate
     * 
     */
    void App_IRTI_getBsp(void* bsp)
    {
        device = (CHAPPIE*)bsp;
    }
    
}

#endif
