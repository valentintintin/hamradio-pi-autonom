import { Observable, of } from 'rxjs';
import { LogService } from './log.service';
import { catchError, map, switchMap, tap } from 'rxjs/operators';
import { RadioService } from './radio.service';
import { SensorsService } from './sensors.service';
import { AprsConfigInterface } from '../config/aprs-config.interface';
import { SensorsConfigInterface } from '../config/sensors-config.interface';
import { DatabaseService } from './database.service';
import { EnumVariable } from '../models/variables';
import { TncService } from './tnc.service';

export class AprsService {

    public static alreadyInUse: boolean;

    public static sendAprsBeacon(config: AprsConfigInterface, keepRadioOn: boolean = false): Observable<void> {
        LogService.log('aprs', 'Start sending APRS Beacon');

        if (AprsService.alreadyInUse) {
            LogService.log('aprs', 'Already in use');
            return of(null);
        }

        AprsService.alreadyInUse = true;

        return RadioService.pttOn().pipe(
            switchMap(_ => {
                LogService.log('aprs', 'Send APRS beacon');

                const latitude = AprsService.convertToDmsAprs(config.lat).toString(10).padStart(7, '0') + 'N';
                const longitude = AprsService.convertToDmsAprs(config.lng).toString(10).padStart(8, '0') + 'E';
                const altitude = Math.round(config.altitude * 3.281).toString(10).padStart(6, '0');

                return TncService.instance.send(config.callSrc, config.callDest,
                    `!${latitude}${config.symbolTable}${longitude}${config.symbolCode}/A=${altitude}${config.comment}`,
                    config.path);
            }),
            switchMap(_ => RadioService.pttOff(!keepRadioOn)),
            tap(_ => {
                AprsService.alreadyInUse = false;
                LogService.log('aprs', 'Send APRS Beacon OK');
            }),
            catchError(err => {
                AprsService.alreadyInUse = false;
                LogService.log('aprs', 'Send APRS Beacon KO', err);
                return RadioService.pttOff(!keepRadioOn);
            }),
        );
    }

    public static sendAprsTelemetry(configAprs: AprsConfigInterface, configSensors: SensorsConfigInterface, keepRadioOn: boolean = false): Observable<void> {
        LogService.log('aprs', 'Start sending APRS Telemetry');

        if (AprsService.alreadyInUse) {
            LogService.log('aprs', 'Already in use');
            return of(null);
        }

        AprsService.alreadyInUse = true;

        return SensorsService.getAllAndSave(configSensors).pipe(
            switchMap(telemetry => RadioService.pttOn().pipe(
                switchMap(_ => DatabaseService.readVariable<number>(EnumVariable.SEQ_TELEMETRY).pipe(map(d => +d))),
                switchMap(seq => {
                    const framesToSend = [
                        `T#${seq.toString().padStart(3, '0')},${telemetry.voltageBattery},${telemetry.currentCharge},${telemetry.temperatureRtc},0,0,00000000`
                    ];

                    if (seq === 0) {
                        framesToSend.unshift(`:${configAprs.callSrc.padEnd(9, ' ')}:PARM.VBat,ICharge,Temp`);
                        framesToSend.unshift(`:${configAprs.callSrc.padEnd(9, ' ')}:UNIT.Volts,Amps,Celcius`);
                    }

                    return TncService.instance.sendMultiples(configAprs.callSrc, configAprs.callDest, framesToSend, configAprs.path).pipe(
                        switchMap(_ => DatabaseService.updateVariable(EnumVariable.SEQ_TELEMETRY, (seq + 1) % 100))
                    );
                }),
                switchMap(_ => RadioService.pttOff(!keepRadioOn)),
                tap(_ => {
                    AprsService.alreadyInUse = false;
                    LogService.log('aprs', 'Send APRS Telemetry OK');
                }),
                catchError(err => {
                    AprsService.alreadyInUse = false;
                    LogService.log('aprs', 'Send APRS Telemetry KO', err);
                    return RadioService.pttOff(!keepRadioOn);
                }),
            ))
        );
    }

    private static convertToDmsAprs(dd: number): number {
        const absDd = Math.abs(dd);
        const deg = Math.floor(absDd);
        const frac = absDd - deg;
        const min = frac * 60;
        return +`${deg}${min.toFixed(2)}`;
    }
}
