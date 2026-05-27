/**
 * @file EmmaThermal.hpp
 * @author Forairaaaaa
 * @brief 
 * @version 0.1
 * @date 2023-03-02
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#pragma once
#include "../../EmmaConfig.h"
#if EMMA_MODULE_Thermal
#include "Thermal/Thermal_Class.hpp"
#include <driver/gpio.h>


// #define MLX_TOUCH_LONG_PUSH_T 200 // touch long push
// #define uS_TO_S_FACTOR 1000000ULL  /* Conversion factor for micro seconds to seconds */

class EmmaThermal : public m5::Thermal_Class {
    private:
    public:
        EmmaThermal()
        {
            /* Setup pins */
            auto cfg = config();
            cfg.PIN_MLX_VDD = EMMA_Thermal_MLX_VDD_PIN;
            cfg.PIN_MLX_SDA = EMMA_Thermal_MLX_SDA_PIN;
            cfg.PIN_MLX_SCL = EMMA_Thermal_MLX_SCL_PIN;
            // cfg.MLX_ADDR    = EMMA_Thermal_MLX_IIC_ADDR;
            cfg.MLX_SHIFT   = MLX_TA_SHIFT;
            config(cfg);
            // begin();

            // enable();
        }
        ~EmmaThermal() { end(); }
        
        // /* For hardware enable pin if exist */
        // inline void enable() { setCtrlPin(1); }
        // inline void disable() { setCtrlPin(0); }
        // inline void setCtrlPin(uint8_t level)
        // {
        //     // gpio_reset_pin((gpio_num_t)EMMA_Thermal_ENABLE_PIN);
        //     // gpio_set_direction((gpio_num_t)EMMA_Thermal_ENABLE_PIN, GPIO_MODE_OUTPUT_OD);
        //     // gpio_set_level((gpio_num_t)EMMA_Thermal_ENABLE_PIN, level);
        // }
        
};

#endif
