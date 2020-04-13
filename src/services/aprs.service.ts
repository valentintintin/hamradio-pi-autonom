import { Observable, Observer } from 'rxjs';
import { LogService } from './log.service';
import { catchError, switchMap } from 'rxjs/operators';
import { RadioService } from './radio.service';
import { SensorsService } from './sensors.service';
import { AprsConfigInterface } from '../config/aprs-config.interface';
import ChildProcess = require('child_process');

export class AprsService {

    public static SEQ_TELEMETRY: number = 0;

    public static sendAprsBeacon(config: AprsConfigInterface, keepRadioOn: boolean = false): Observable<void> {
        LogService.log('aprs', 'Start sending APRS Beacon');

        return RadioService.pttOn().pipe(
            switchMap(_ => {
                return new Observable<void>((observer: Observer<void>) => {
                    try {
                        LogService.log('aprs', 'Send APRS Beacon');

                        ChildProcess.execFileSync(config.ax25beaconPath + '/ax25beacon', [
                            '-s ' + config.callSrc,
                            '-d ' + (config.callDest ? config.callDest : 'APRS'),
                            '-p ' + (config.path ? config.path : 'WIDE1-1,WIDE2-1'),
                            '-t ' + (config.symbolTable ? config.symbolTable : '/'),
                            '-c ' + (config.symbolCode ? config.symbolCode : '"'),
                            '' + config.lat,
                            '' + config.lng,
                            config.altitude ? '' + config.altitude : '0',
                            config.comment ? config.comment : ''
                        ], {
                            encoding: 'utf8'
                        });
                        observer.next();
                        observer.complete();
                    } catch (e) {
                        observer.error(e);
                    }
                });
            }),
            switchMap(_ => RadioService.pttOff(!keepRadioOn)),
            catchError(err => {
                LogService.log('aprs', 'Send APRS Beacon KO', err);
                return RadioService.pttOff(!keepRadioOn);
            }),
        );
    }

    public static sendAprsTelemetry(config: AprsConfigInterface = null, keepRadioOn: boolean = false): Observable<void> {
        LogService.log('aprs', 'Start sending APRS Telemetry');

        return SensorsService.getAll().pipe(
            switchMap(telemetry => RadioService.pttOn().pipe(
                switchMap(_ => {
                    return new Observable<void>((observer: Observer<void>) => {
                        if (this.SEQ_TELEMETRY === 0) {
                            try {
                                LogService.log('aprs', 'Send APRS Telemetry Name');

                                ChildProcess.execFileSync(config.ax25beaconPath + '/ax25frame', [
                                    '-s ' + config.callSrc,
                                    '-d ' + (config.callDest ? config.callDest : 'APRS'),
                                    '-p ' + (config.path ? config.path : 'WIDE1-1,WIDE2-1'),
                                    `:${config.callSrc}   :PARM,VBat,ICharge,Temp`
                                ], {
                                    encoding: 'utf8'
                                });
                            } catch (e) {
                                LogService.log('aprs', 'Send APRS Telemetry Name KO', e);
                            }
                            try {
                                LogService.log('aprs', 'Send APRS Telemetry Unit');

                                ChildProcess.execFileSync(config.ax25beaconPath + '/ax25frame', [
                                    '-s ' + config.callSrc,
                                    '-d ' + (config.callDest ? config.callDest : 'APRS'),
                                    '-p ' + (config.path ? config.path : 'WIDE1-1,WIDE2-1'),
                                    `:${config.callSrc}   :UNIT,Volts,Amps,Celcius`
                                ], {
                                    encoding: 'utf8'
                                });
                            } catch (e) {
                                LogService.log('aprs', 'Send APRS Telemetry Unit KO', e);
                            }
                        }

                        try {
                            LogService.log('aprs', 'Send APRS Telemetry');

                            ChildProcess.execFileSync(config.ax25beaconPath + '/ax25frame', [
                                '-s ' + config.callSrc,
                                '-d ' + (config.callDest ? config.callDest : 'APRS'),
                                '-p ' + (config.path ? config.path : 'WIDE1-1,WIDE2-1'),
                                `T#${AprsService.SEQ_TELEMETRY.toString().padStart(2, '0')},${telemetry.voltageBatterie},${telemetry.intensiteCharge},${telemetry.temperatureRtc}`
                            ], {
                                encoding: 'utf8'
                            });

                            AprsService.SEQ_TELEMETRY++;
                            AprsService.SEQ_TELEMETRY %= 1000;

                            observer.next();
                            observer.complete();
                        } catch (e) {
                            observer.error(e);
                        }
                    });
                }),
                switchMap(_ => RadioService.pttOff(!keepRadioOn)),
                catchError(err => {
                    LogService.log('aprs', 'Send APRS Telemetry KO', err);
                    return RadioService.pttOff(!keepRadioOn);
                }),
            ))
        );
    }
}
