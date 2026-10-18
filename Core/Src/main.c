/* USER CODE BEGIN Header */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dac.h"
#include "dma.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private variables ---------------------------------------------------------*/
#define QUANTIDADE_AMOSTRA 400
#define BLOCO_AMOSTRA      (QUANTIDADE_AMOSTRA / 2)
#define ADC_MIDPOINT       2048.0f
#define DAC_MAX_VALUE      4095
#define COMB_DELAY_SAMPLES 200
#define COMB_FEEDBACK      0.85f
static uint16_t adc[QUANTIDADE_AMOSTRA];
static uint16_t saida[QUANTIDADE_AMOSTRA];

static float comb_delay[COMB_DELAY_SAMPLES];
static uint32_t comb_index = 0;

void SystemClock_Config(void);
void Error_Handler(void);

/* USER CODE BEGIN 0 */
/*
 */

static void comb_filter_init(void)
{
    for (uint32_t i = 0; i < COMB_DELAY_SAMPLES; i++)
    {
        comb_delay[i] = 0.0f;
    }
    comb_index = 0;
}

static uint16_t comb_filter_process(uint16_t adc_sample)
{
    float x;
    float delayed;
    float internal_y;
    float y;
    int32_t dac_value;

    x = (float)adc_sample - ADC_MIDPOINT;

    delayed = comb_delay[comb_index];

    internal_y = x + COMB_FEEDBACK * delayed;

    comb_delay[comb_index] = internal_y;

    comb_index++;

    if (comb_index >= COMB_DELAY_SAMPLES)
    {
        comb_index = 0;
    }


    y = (1.0f - COMB_FEEDBACK) * internal_y;

    dac_value = (int32_t)(y + ADC_MIDPOINT);

    if (dac_value < 0)
    {
        dac_value = 0;
    }
    else if (dac_value > DAC_MAX_VALUE)
    {
        dac_value = DAC_MAX_VALUE;
    }

    return (uint16_t)dac_value;
}


static void process_audio_block(uint32_t start, uint32_t count)
{
    for (uint32_t i = start; i < start + count; i++)
    {
        saida[i] = comb_filter_process(adc[i]);
    }
}

/* USER CODE END 0 */

/**
 * @brief Main program
 */
int main(void)
{
    HAL_Init();

    SystemClock_Config();

    MX_GPIO_Init();
    MX_DMA_Init();
    MX_USART2_UART_Init();
    MX_ADC1_Init();
    MX_DAC_Init();
    MX_TIM8_Init();

    /* USER CODE BEGIN 2 */


    for (uint32_t i = 0; i < QUANTIDADE_AMOSTRA; i++)
    {
        adc[i] = 2048;
        saida[i] = 2048;
    }

    comb_filter_init();


    if (HAL_DAC_Start_DMA(
            &hdac,
            DAC_CHANNEL_1,
            (uint32_t *)saida,
            QUANTIDADE_AMOSTRA,
            DAC_ALIGN_12B_R) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_ADC_Start_DMA(
            &hadc1,
            (uint32_t *)adc,
            QUANTIDADE_AMOSTRA) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_TIM_Base_Start(&htim8) != HAL_OK)
    {
        Error_Handler();
    }

    /* USER CODE END 2 */

    while (1)
    {

    }
}

/**
 * @brief System Clock Configuration
 */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM = 16;
    RCC_OscInitStruct.PLL.PLLN = 336;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
    RCC_OscInitStruct.PLL.PLLQ = 2;
    RCC_OscInitStruct.PLL.PLLR = 2;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK |
                                  RCC_CLOCKTYPE_SYSCLK |
                                  RCC_CLOCKTYPE_PCLK1 |
                                  RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
    {
        Error_Handler();
    }
}

/* USER CODE BEGIN 4 */


void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1)
    {
        process_audio_block(0, BLOCO_AMOSTRA);
    }
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1)
    {
        process_audio_block(BLOCO_AMOSTRA, BLOCO_AMOSTRA);
    }
}

/* USER CODE END 4 */

void Error_Handler(void)
{
    __disable_irq();

    while (1)
    {
    }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{

}
#endif
