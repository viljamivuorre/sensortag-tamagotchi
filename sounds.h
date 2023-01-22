
/*
 * Viljami Vuorre
 * Toivo Xiong
 * Riku Kyllönen
 */
#ifndef SOUNDS_H_
#define SOUNDS_H_
#include "buzzer.h"
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Clock.h>

#include <xdc/std.h>
#include <xdc/runtime/System.h>

enum aanet {SILENCE=0,ONEBEEP,TWOBEEP,MYSTICALTREE};
enum aanet soundState=SILENCE;

static PIN_Handle hPin;


Void soundTask(UArg arg0, UArg arg1) {                                                                                                                                                                                                                                                                                      // int k = !
                                                                                                                                                                                                                                                                                                                            //if (k == 1) {    lololololol
                                                                                                                                                                                                                                                                                                                                // k = 1}
   while (1) {

       switch (soundState) {

      case SILENCE: //this plays silence
          break;

      case ONEBEEP: //this plays one beeper
          buzzerOpen(hPin);
          buzzerSetFrequency(600);
          Task_sleep(100000 / Clock_tickPeriod);
          buzzerSetFrequency(0);
          buzzerClose();
          soundState = SILENCE;
          break;

      case TWOBEEP: //this plays twoo beeps
          buzzerOpen(hPin);
          buzzerSetFrequency(1000);
          Task_sleep(100000 / Clock_tickPeriod);
          buzzerSetFrequency(0);
          Task_sleep(100000 / Clock_tickPeriod);
          buzzerSetFrequency(700);
          Task_sleep(100000 / Clock_tickPeriod);
          buzzerSetFrequency(0);
          buzzerClose();
          soundState = SILENCE;
          break;


      case MYSTICALTREE: //this plays mystical tree
          buzzerOpen(hPin);
          buzzerSetFrequency(783);
          Task_sleep(500000*0.5 / Clock_tickPeriod);

          buzzerSetFrequency(392.00);
          Task_sleep(500000*0.5 / Clock_tickPeriod);

          buzzerSetFrequency(783.99);
          Task_sleep(500000 / Clock_tickPeriod);

          buzzerSetFrequency(698.46);
          Task_sleep(500000*0.5 / Clock_tickPeriod);

          buzzerSetFrequency(587.33);
          Task_sleep(500000*0.5 / Clock_tickPeriod);

          buzzerSetFrequency(392.00);
          Task_sleep(500000*0.5 / Clock_tickPeriod);

          buzzerSetFrequency(523.25);
          Task_sleep(500000*0.5 / Clock_tickPeriod);

          buzzerSetFrequency(622.25);
          Task_sleep(500000 / Clock_tickPeriod);

          buzzerSetFrequency(587.33);
          Task_sleep(500000*0.5 / Clock_tickPeriod);

          buzzerSetFrequency(466.16);
          Task_sleep(500000*0.5 / Clock_tickPeriod);

          buzzerSetFrequency(311.13);
          Task_sleep(500000*0.5 / Clock_tickPeriod);

          buzzerSetFrequency(392.00);
          Task_sleep(500000*0.5 / Clock_tickPeriod);

          buzzerSetFrequency(523.25);
          Task_sleep(500000 / Clock_tickPeriod);

          buzzerSetFrequency(466.16);
          Task_sleep(500000*0.5 / Clock_tickPeriod);

          buzzerSetFrequency(392.00);
          Task_sleep(500000*0.5 / Clock_tickPeriod);

          buzzerSetFrequency(523.25);
          Task_sleep(500000*3 / Clock_tickPeriod);

          buzzerSetFrequency(0);
          buzzerClose();
          soundState = SILENCE;
          break;

      default:
//              soundState=SILENCE;

          break;
      }
      Task_sleep(200000L / Clock_tickPeriod);
   }
}


#endif /* AANET_H_ */
