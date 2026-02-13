Alex Belliard and Joey Cerulli
ECE-218 - Embedded Microcontroller Projects
Instructor: Cherrice Traver
2/12/26

Project 3 - Windshield Wiper Subsystem

System Behavior
Similar to the original implementation of the base car design, the driver must be seated for the system to begin. 
Once the driver is seated, a welcome message prints, the system will wait for the passenger to sit, and for both people in the car to put
on their seatbelts. When all the conditions are met, a green LED will light indicating the ignition is ready to 
be pressed. Once the ignition is pressed, a red LED will light signifying the engine started successfully and a success message prints. If the 
engine fails to start because at least one condition wasn't met, a buzzer would go off and messages would print 
indicating the errors that inhibited the system from starting. From here the user can attempt to start the car 
again by sitting back in the driver seat. Our newest addition to the system is a windsheild wiper system that will 
run as long as the ignition is enabled. It features 4 possible modes in the following order: off, high, low, 
interval. The interval setting will allow the wipers to sweep at the low speed with varying interval options in the 
following order: short, medium, high. This will determine the time in between each sweep (short for quicker, long 
for slower, etc.).


Design Alternatives



╔══════════════════════════════════════════════════════════════════════════════════════════════════════════════════╗
║                                                Ignition Subsystem                                                ║
╠═════════════════════════════════╦══════════════════════════════════════════╦═════════════════════════════════════╣
║          Specification          ║               Test Process               ║               Results               ║
╠═════════════════════════════════╬══════════════════════════════════════════╬═════════════════════════════════════╣
║ Enable engine start (i.e.,      ║ 4 buttons:                               ║ All tests passed.                   ║
║ light the green LED) while      ║ DRIVER_OCC                               ║ 1.Green light on                    ║
║ both seats are occupied         ║ PASS_OCC                                 ║ 2.“Passenger seatbelt not fastened” ║
║ (DRIVER_OCC, PASS_OCC)          ║ DRIVER_BELT                              ║ 3. All error messages printed.      ║
║ and seatbelts fastened          ║ PASS_BELT                                ║                                     ║
║ (DRIVER_BELT, PASS_BELT).       ║ 1. All buttons pressed                   ║                                     ║
║ Otherwise print appropriate     ║ 2. All but one button (PASS_OCC) pressed ║                                     ║
║ error messages.                 ║ 3. No buttons pressed                    ║                                     ║
╠═════════════════════════════════╬══════════════════════════════════════════╬═════════════════════════════════════╣
║ Start the engine (i.e., light   ║ All buttons pressed, green               ║ Test passed.                        ║
║ the yellow LED, turn off Green) ║ LED is on, ignition button               ║ Engine light illuminates and        ║
║ when ignition is enabled (green ║ pressed                                  ║ stays on after ignition button      ║
║ LED) and ignition button is     ║                                          ║ is released.                        ║
║ pressed  (i.e., before the      ║                                          ║                                     ║
║ button is released).            ║                                          ║                                     ║
╠═════════════════════════════════╬══════════════════════════════════════════╬═════════════════════════════════════╣
║ Test ignition turning off       ║ Set all requirements and turn on the     ║ Test passed.                        ║
║ the car.                        ║ car. Click the ignition button again     ║ When the ignition was pressed       ║
║                                 ║ and check if the engine light turns off. ║ again, the engine light shuts off.  ║
╠═════════════════════════════════╬══════════════════════════════════════════╬═════════════════════════════════════╣
║ Test that engine stays on while ║ Set all requirements and turn on the     ║ Test passed.                        ║
║ requirements are changed after  ║ car, so that the engine light is         ║ When requirements were altered      ║
║ the car is already on.          ║ illuminated. Then change some of the     ║ after the engine button was         ║
║                                 ║ requirements and make sure the engine    ║ already on, the light stayed on.    ║
║                                 ║ button stays illuminated.                ║                                     ║
╠═════════════════════════════════╩══════════════════════════════════════════╩═════════════════════════════════════╣
║                                                Windshield subsystem                                              ║
╠═════════════════════════════════╦══════════════════════════════════════════╦═════════════════════════════════════╣
║ Only works while the engine     ║ - Turn on car and test that wipers       ║ All tests passed.                   ║
║ is running.                     ║ are fully functional.                    ║ - When car is on wipers will        ║
║                                 ║                                          ║ turn on after potentiometer is      ║
║                                 ║                                          ║ turned.                             ║
║                                 ║                                          ║                                     ║
║                                 ║ - Turn car off and make sure wipers      ║ - When the wipers are running and   ║
║                                 ║ are not able to turn on or change modes. ║ the carturns off, wipers turn off   ║
╠═════════════════════════════════╬══════════════════════════════════════════╬═════════════════════════════════════╣
║ Test off                        ║ - When the potentiometer reads the       ║ All tests passed.                   ║
║                                 ║ correct value, the wipers should not     ║ - When the potentiometer is spun    ║
║                                 ║ spin.                                    ║ the wipers turn off.                ║
║                                 ║                                          ║                                     ║
║                                 ║ - When off, Mode will print but interval ║ - When potentiometer is spun the    ║
║                                 ║ will be empty.                           ║ Mode switches to OFF and the        ║
║                                 ║                                          ║ Interval is empty.                  ║
╠═════════════════════════════════╬══════════════════════════════════════════╬═════════════════════════════════════╣
║ Test high                       ║ - Turn potentiometer to high and check   ║ All tests passed.                   ║
║                                 ║ that the wipers run at 25 rpm.           ║ - When potentiometer is high the    ║
║                                 ║                                          ║ wipers run at 25 rpm, or 1.5 degrees║
║                                 ║                                          ║ per second.                         ║
║                                 ║ - Turn potentiometer to high and check   ║ - When potentiometer is high        ║
║                                 ║ that the wipers run at a 90 degree angle ║ the wipers go 90 degrees both ways. ║
╠═════════════════════════════════╬══════════════════════════════════════════╬═════════════════════════════════════╣
║ Test low                        ║ - Turn potentiometer to low and check    ║ All tests passed.                   ║
║                                 ║ that the wipers run at 10 rpm.           ║ - When potentiometer is low the     ║
║                                 ║                                          ║ wipers run at 10 rpm, 1 degree per  ║
║                                 ║                                          ║ second.                             ║
║                                 ║ - Turn potentiometer to high and check   ║ - When potentiometer is low         ║
║                                 ║ that the wipers run at a 90 degree angle ║ the wipers go 90 degrees both ways. ║
╠═════════════════════════════════╬══════════════════════════════════════════╬═════════════════════════════════════╣
║ Test interval: short            ║ - Turn potentiometer to interval and     ║ All tests passed.                   ║
║                                 ║ check that that the headlights are on               ║ - When potentiometer is high        ║
║                                 ║                                          ║ the lights are on                   ║
║               fix                   ║ - Turn potentiometer to low and check    ║                                     ║
║                                 ║ that the headlights are off              ║ - When potentiometer is low         ║
║                                 ║                                          ║ the lights are off                  ║
╠═════════════════════════════════╬══════════════════════════════════════════╬═════════════════════════════════════╣
║ Test interval: med              ║ - Turn potentiometer to high and check   ║ All tests passed.                   ║
║                                 ║ that the headlights are on               ║ - When potentiometer is high        ║
║                                 ║                                          ║ the lights are on                   ║
║                  fix                ║ - Turn potentiometer to low and check    ║                                     ║
║                                 ║ that the headlights are off              ║ - When potentiometer is low         ║
║                                 ║                                          ║ the lights are off                  ║
╠═════════════════════════════════╬══════════════════════════════════════════╬═════════════════════════════════════╣
║ Test interval: high             ║ - Turn potentiometer to high and check   ║ All tests passed.                   ║
║                                 ║ that the headlights are on               ║ - When potentiometer is high        ║
║                                 ║                                          ║ the lights are on                   ║
║                  fix                ║ - Turn potentiometer to low and check    ║                                     ║
║                                 ║ that the headlights are off              ║ - When potentiometer is low         ║
║     add test for 90 degree              ║                                          ║ the lights are off                  ║
╚═════════════════════════════════╩══════════════════════════════════════════╩═════════════════════════════════════╝
