/**
 * Copyright (c) 2016 - 2019, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
/** @file
 * @defgroup fatfs_example_main main.c
 * @{
 * @ingroup fatfs_example
 * @brief FATFS Example Application main file.
 *
 * This file contains the source code for a sample application using FAT filesystem and SD card library.
 *
 */


#include "nrf.h"
#include "bsp.h"
#include "ff.h"
#include "diskio_blkdev.h"
#include "nrf_block_dev_sdc.h"


#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "nrf_delay.h"

#define FILE_NAME   "NORDIC.TXT"
#define TEST_STRING "SD card example."

#define SDC_SCK_PIN     ARDUINO_13_PIN  ///< SDC serial clock (SCK) pin.
#define SDC_MOSI_PIN    ARDUINO_11_PIN  ///< SDC serial data in (DI) pin.
#define SDC_MISO_PIN    ARDUINO_12_PIN  ///< SDC serial data out (DO) pin.
#define SDC_CS_PIN      ARDUINO_10_PIN  ///< SDC chip select (CS) pin.

#define OID_DEC_SCK     NRF_GPIO_PIN_MAP(1, 6)
#define OID_DEC_SDO     NRF_GPIO_PIN_MAP(1, 8)
#define OID_DEC_DELAY   50

#define HIGH            1
#define LOW             0

#define OUTPUT          0xf0
#define INPUT           0xf1

/**
 * @brief  SDC block device definition
 * */
NRF_BLOCK_DEV_SDC_DEFINE(
        m_block_dev_sdc,
        NRF_BLOCK_DEV_SDC_CONFIG(
                SDC_SECTOR_SIZE,
                APP_SDCARD_CONFIG(SDC_MOSI_PIN, SDC_MISO_PIN, SDC_SCK_PIN, SDC_CS_PIN)
         ),
         NFR_BLOCK_DEV_INFO_CONFIG("Nordic", "SDC", "1.00")
);

/**
 * @brief Function for demonstrating FAFTS usage.
 */
static void fatfs_example()
{
    static FATFS fs;
    static DIR dir;
    static FILINFO fno;
    static FIL file;

    uint32_t bytes_written;
    FRESULT ff_result;
    DSTATUS disk_state = STA_NOINIT;

    // Initialize FATFS disk I/O interface by providing the block device.
    static diskio_blkdev_t drives[] =
    {
            DISKIO_BLOCKDEV_CONFIG(NRF_BLOCKDEV_BASE_ADDR(m_block_dev_sdc, block_dev), NULL)
    };

    diskio_blockdev_register(drives, ARRAY_SIZE(drives));

    printf("Initializing disk 0 (SDC)...\r\n");
    for (uint32_t retries = 3; retries && disk_state; --retries)
    {
        disk_state = disk_initialize(0);
    }
    if (disk_state)
    {
        printf("Disk initialization failed.\r\n");
        return;
    }

    uint32_t blocks_per_mb = (1024uL * 1024uL) / m_block_dev_sdc.block_dev.p_ops->geometry(&m_block_dev_sdc.block_dev)->blk_size;
    uint32_t capacity = m_block_dev_sdc.block_dev.p_ops->geometry(&m_block_dev_sdc.block_dev)->blk_count / blocks_per_mb;
    printf("Capacity: %d MB\r\n", capacity);

    printf("Mounting volume...\r\n");
    ff_result = f_mount(&fs, "", 1);
    if (ff_result)
    {
        printf("Mount failed.\r\n");
        return;
    }

    printf("\r\n Listing directory: /");
    ff_result = f_opendir(&dir, "/");
    if (ff_result)
    {
        printf("Directory listing failed!\r\n");
        return;
    }

    do
    {
        ff_result = f_readdir(&dir, &fno);
        if (ff_result != FR_OK)
        {
            printf("Directory read failed.\r\n");
            return;
        }

        if (fno.fname[0])
        {
            if (fno.fattrib & AM_DIR)
            {
                printf("   <DIR>   %s\r\n",(uint32_t)fno.fname);
            }
            else
            {
                printf("%9lu  %s\r\n", fno.fsize, (uint32_t)fno.fname);
            }
        }
    }
    while (fno.fname[0]);
    NRF_LOG_RAW_INFO("");

    printf("Writing to file \r\n" FILE_NAME "...");
    ff_result = f_open(&file, FILE_NAME, FA_READ | FA_WRITE | FA_OPEN_APPEND);
    if (ff_result != FR_OK)
    {
        printf("Unable to open or create file: " FILE_NAME ".\r\n");
        return;
    }

    ff_result = f_write(&file, TEST_STRING, sizeof(TEST_STRING) - 1, (UINT *) &bytes_written);
    if (ff_result != FR_OK)
    {
        printf("Write failed\r\n.");
    }
    else
    {
        printf("%d bytes written.\r\n", bytes_written);
    }

    (void) f_close(&file);
    return;
}

void digitalWrite(uint8_t pin, uint8_t value)
{
  if(value)
    nrf_gpio_pin_set(pin);
  else 
    nrf_gpio_pin_clear(pin);
}

uint8_t digitalRead(uint8_t pin)
{
  return (nrf_gpio_pin_read(pin))?HIGH:LOW;
}

void board_init()
{
  nrf_gpio_cfg_input(OID_DEC_SDO, NRF_GPIO_PIN_NOPULL);
  nrf_gpio_cfg_output(OID_DEC_SCK);

  digitalWrite(OID_DEC_SCK, HIGH);
          
  nrf_delay_ms(30);
  digitalWrite(OID_DEC_SCK, LOW);
  nrf_delay_ms(10);
}

void readSDIOData(){
	 //SEGGER_RTT_printf(0, "oid\n");
   uint32_t dataTemp1= 0, newRecordedDaata = 0;
   newRecordedDaata = 0xFFFFFF;
   
   digitalWrite(OID_DEC_SCK, HIGH);   // start
   nrf_delay_us(OID_DEC_DELAY);
  
   nrf_gpio_cfg_output(OID_DEC_SDO);
   digitalWrite(OID_DEC_SDO, LOW);
   digitalWrite(OID_DEC_SCK, LOW);

   nrf_delay_us(OID_DEC_DELAY);   

   nrf_gpio_cfg_input(OID_DEC_SDO, NRF_GPIO_PIN_NOPULL);
   
   digitalWrite(OID_DEC_SCK, HIGH);    // 22th Bit   
   nrf_delay_us(OID_DEC_DELAY);   
   digitalWrite(OID_DEC_SCK, LOW);     
   nrf_delay_us(OID_DEC_DELAY);
   dataTemp1 = dataTemp1 << 1;
   if (digitalRead(OID_DEC_SDO) == HIGH) dataTemp1 = dataTemp1 + 1;
   
   digitalWrite(OID_DEC_SCK, HIGH);    // 21th Bit
   nrf_delay_us(OID_DEC_DELAY);
   digitalWrite(OID_DEC_SCK, LOW);     
   nrf_delay_us(OID_DEC_DELAY);
   dataTemp1 = dataTemp1 << 1;
   if (digitalRead(OID_DEC_SDO) == HIGH) dataTemp1 = dataTemp1 + 1;

   digitalWrite(OID_DEC_SCK, HIGH);    // 20th Bit
   nrf_delay_us(OID_DEC_DELAY);
   digitalWrite(OID_DEC_SCK, LOW);     
   nrf_delay_us(OID_DEC_DELAY);
   dataTemp1 = dataTemp1 << 1;
   if (digitalRead(OID_DEC_SDO) == HIGH) dataTemp1 = dataTemp1 + 1;

   digitalWrite(OID_DEC_SCK, HIGH);    // 19th Bit
   nrf_delay_us(OID_DEC_DELAY);
   digitalWrite(OID_DEC_SCK, LOW);     
   nrf_delay_us(OID_DEC_DELAY);
   dataTemp1 = dataTemp1 << 1;
   if (digitalRead(OID_DEC_SDO) == HIGH) dataTemp1 = dataTemp1 + 1;

   digitalWrite(OID_DEC_SCK, HIGH);    // 18th Bit
   nrf_delay_us(OID_DEC_DELAY);
   digitalWrite(OID_DEC_SCK, LOW);     
   nrf_delay_us(OID_DEC_DELAY);
   dataTemp1 = dataTemp1 << 1;
   if (digitalRead(OID_DEC_SDO) == HIGH) dataTemp1 = dataTemp1 + 1;

   digitalWrite(OID_DEC_SCK, HIGH);    // 17th Bit
   nrf_delay_us(OID_DEC_DELAY);
   digitalWrite(OID_DEC_SCK, LOW);     
   nrf_delay_us(OID_DEC_DELAY);
   dataTemp1 = dataTemp1 << 1;
   if (digitalRead(OID_DEC_SDO) == HIGH) dataTemp1 = dataTemp1 + 1;

   digitalWrite(OID_DEC_SCK, HIGH);    // 16th Bit
   nrf_delay_us(OID_DEC_DELAY);
   digitalWrite(OID_DEC_SCK, LOW);     
   nrf_delay_us(OID_DEC_DELAY);
   dataTemp1 = dataTemp1 << 1;
   if (digitalRead(OID_DEC_SDO) == HIGH) dataTemp1 = dataTemp1 + 1;

   digitalWrite(OID_DEC_SCK, HIGH);    // 15th Bit
   nrf_delay_us(OID_DEC_DELAY);
   digitalWrite(OID_DEC_SCK, LOW);     
   nrf_delay_us(OID_DEC_DELAY);
   dataTemp1 = dataTemp1 << 1;
   if (digitalRead(OID_DEC_SDO) == HIGH) dataTemp1 = dataTemp1 + 1;

   digitalWrite(OID_DEC_SCK, HIGH);    // 14th Bit
   nrf_delay_us(OID_DEC_DELAY);
   digitalWrite(OID_DEC_SCK, LOW);     
   nrf_delay_us(OID_DEC_DELAY);
   dataTemp1 = dataTemp1 << 1;
   if (digitalRead(OID_DEC_SDO) == HIGH) dataTemp1 = dataTemp1 + 1;

   digitalWrite(OID_DEC_SCK, HIGH);    // 13th Bit
   nrf_delay_us(OID_DEC_DELAY);
   digitalWrite(OID_DEC_SCK, LOW);     
   nrf_delay_us(OID_DEC_DELAY);
   dataTemp1 = dataTemp1 << 1;
   if (digitalRead(OID_DEC_SDO) == HIGH) dataTemp1 = dataTemp1 + 1;

   digitalWrite(OID_DEC_SCK, HIGH);    // 12th Bit
   nrf_delay_us(OID_DEC_DELAY);
   digitalWrite(OID_DEC_SCK, LOW);     
   nrf_delay_us(OID_DEC_DELAY);
   dataTemp1 = dataTemp1 << 1;
   if (digitalRead(OID_DEC_SDO) == HIGH) dataTemp1 = dataTemp1 + 1;

   digitalWrite(OID_DEC_SCK, HIGH);    // 11th Bit
   nrf_delay_us(OID_DEC_DELAY);
   digitalWrite(OID_DEC_SCK, LOW);     
   nrf_delay_us(OID_DEC_DELAY);
   dataTemp1 = dataTemp1 << 1;
   if (digitalRead(OID_DEC_SDO) == HIGH) dataTemp1 = dataTemp1 + 1;

   digitalWrite(OID_DEC_SCK, HIGH);    // 10th Bit
   nrf_delay_us(OID_DEC_DELAY);
   digitalWrite(OID_DEC_SCK, LOW);     
   nrf_delay_us(OID_DEC_DELAY);
   dataTemp1 = dataTemp1 << 1;
   if (digitalRead(OID_DEC_SDO) == HIGH) dataTemp1 = dataTemp1 + 1;

   digitalWrite(OID_DEC_SCK, HIGH);    // 9th Bit
   nrf_delay_us(OID_DEC_DELAY);
   digitalWrite(OID_DEC_SCK, LOW);     
   nrf_delay_us(OID_DEC_DELAY);
   dataTemp1 = dataTemp1 << 1;
   if (digitalRead(OID_DEC_SDO) == HIGH) dataTemp1 = dataTemp1 + 1;

   digitalWrite(OID_DEC_SCK, HIGH);    // 8th Bit
   nrf_delay_us(OID_DEC_DELAY);
   digitalWrite(OID_DEC_SCK, LOW);     
   nrf_delay_us(OID_DEC_DELAY);
   dataTemp1 = dataTemp1 << 1;
   if (digitalRead(OID_DEC_SDO) == HIGH) dataTemp1 = dataTemp1 + 1;

   digitalWrite(OID_DEC_SCK, HIGH);    // 7th Bit
   nrf_delay_us(OID_DEC_DELAY);
   digitalWrite(OID_DEC_SCK, LOW);     
   nrf_delay_us(OID_DEC_DELAY);
   dataTemp1 = dataTemp1 << 1;
   if (digitalRead(OID_DEC_SDO) == HIGH) dataTemp1 = dataTemp1 + 1;

   digitalWrite(OID_DEC_SCK, HIGH);    // 6th Bit
   nrf_delay_us(OID_DEC_DELAY);
   digitalWrite(OID_DEC_SCK, LOW);     
   nrf_delay_us(OID_DEC_DELAY);
   dataTemp1 = dataTemp1 << 1;
   if (digitalRead(OID_DEC_SDO) == HIGH) dataTemp1 = dataTemp1 + 1;

   digitalWrite(OID_DEC_SCK, HIGH);    // 5th Bit
   nrf_delay_us(OID_DEC_DELAY);
   digitalWrite(OID_DEC_SCK, LOW);     
   nrf_delay_us(OID_DEC_DELAY);
   dataTemp1 = dataTemp1 << 1;
   if (digitalRead(OID_DEC_SDO) == HIGH) dataTemp1 = dataTemp1 + 1;

   digitalWrite(OID_DEC_SCK, HIGH);    // 4th Bit
   nrf_delay_us(OID_DEC_DELAY);
   digitalWrite(OID_DEC_SCK, LOW);     
   nrf_delay_us(OID_DEC_DELAY);
   dataTemp1 = dataTemp1 << 1;
   if (digitalRead(OID_DEC_SDO) == HIGH) dataTemp1 = dataTemp1 + 1;

   digitalWrite(OID_DEC_SCK, HIGH);    // 3th Bit
   nrf_delay_us(OID_DEC_DELAY);
   digitalWrite(OID_DEC_SCK, LOW);     
   nrf_delay_us(OID_DEC_DELAY);
   dataTemp1 = dataTemp1 << 1;
   if (digitalRead(OID_DEC_SDO) == HIGH) dataTemp1 = dataTemp1 + 1;

   digitalWrite(OID_DEC_SCK, HIGH);    // 2th Bit
   nrf_delay_us(OID_DEC_DELAY);
   digitalWrite(OID_DEC_SCK, LOW);     
   nrf_delay_us(OID_DEC_DELAY);
   dataTemp1 = dataTemp1 << 1;
   if (digitalRead(OID_DEC_SDO) == HIGH) dataTemp1 = dataTemp1 + 1;

   digitalWrite(OID_DEC_SCK, HIGH);    // 1th Bit
   nrf_delay_us(OID_DEC_DELAY);
   digitalWrite(OID_DEC_SCK, LOW);     
   nrf_delay_us(OID_DEC_DELAY);
   dataTemp1 = dataTemp1 << 1;
   if (digitalRead(OID_DEC_SDO) == HIGH) dataTemp1 = dataTemp1 + 1;

   digitalWrite(OID_DEC_SCK, HIGH);    // 0th Bit
   nrf_delay_us(OID_DEC_DELAY);
   digitalWrite(OID_DEC_SCK, LOW);     
   nrf_delay_us(OID_DEC_DELAY);
   dataTemp1 = dataTemp1 << 1;
   if (digitalRead(OID_DEC_SDO) == HIGH) dataTemp1 = dataTemp1 + 1;
	//	SEGGER_RTT_printf(0, "code %x\r\n", dataTemp1);
//		newRecordedDaata =  dataTemp1;
    printf("%x\r\n", dataTemp1);
	
}
/**
 * @brief Function for main application entry.
 */
int main(void)
{
    bsp_board_init(BSP_INIT_LEDS);
    APP_ERROR_CHECK(NRF_LOG_INIT(NULL));
    NRF_LOG_DEFAULT_BACKENDS_INIT();

    printf("FATFS example started.\r\n");
    
    fatfs_example();
    nrf_delay_ms(100);
    board_init();

    while (true)
    {
      if ((digitalRead(OID_DEC_SDO) == LOW)) 
      {
        readSDIOData();
        nrf_delay_ms(100);
      }
      NRF_LOG_FLUSH();
//      __WFE();
    }
}

/** @} */
