import { ConfigInterface } from '../config/config.interface';
import { LogService } from './log.service';
import { MpptchgService } from './mpptchg.service';
import { AprsService } from './aprs.service';
import { SensorsService } from './sensors.service';
import { of, Subscription, timer } from 'rxjs';
import { catchError, skip, switchMap, tap } from 'rxjs/operators';
import { VoiceService } from './voice.service';
import { AudioDecoder, MultimonModeEnum } from 'nodejs-arecord-multimon';
import { WebcamService } from './webcam.service';
import { SstvService } from './sstv.service';
import { RadioService } from './radio.service';
import { DatabaseService } from './database.service';
import { ApiService } from './api.service';

const ON_DEATH = require('death');

export class ProcessService {

    private audioDecoder = new AudioDecoder();
    private mpptchgSubscription: Subscription;
    private api: ApiService;

    public run(config: ConfigInterface): void {
        LogService.log('program', 'Started');

        if (config.mpptChd && config.mpptChd.enable) {
            this.runMpptChd(config);
        }

        if (config.aprs && config.aprs.enable) {
            this.runAprs(config);
        }

        if (config.sensors && config.sensors.enable) {
            this.runSensors(config);
        }

        if (config.webcam && config.webcam.enable) {
            this.runWebcam(config);
        }

        if (config.api && config.api.enable) {
            this.runApi(config);
        }

        ON_DEATH((signal: any, err: any) => {
            LogService.log('program', 'Stopping');

            let stop = of(null);

            if (this.mpptchgSubscription) {
                this.mpptchgSubscription.unsubscribe();

                stop = stop.pipe(
                    switchMap(_ =>
                        MpptchgService.stop().pipe(
                            tap(_ => LogService.log('mpptChd', 'Watchdog disabled if enabled')),
                            catchError(e => {
                                LogService.log('mpptChd', 'Watchdog impossible to disabled (if enabled) !');
                                return of(null);
                            })
                        )
                    )
                );
            }

            stop.pipe(
                switchMap(_ => DatabaseService.close()),
                catchError(_ => process.exit(1))
            ).subscribe(_ => process.exit(err ? 1 : 0))
        });
    }

    private runMpptChd(config: ConfigInterface): void {
        MpptchgService.battery(config).subscribe();
    }

    private runAprs(config: ConfigInterface): void {
        LogService.log('aprs', 'Started');
        timer(60000, 1000 * (config.aprs.interval ? config.aprs.interval : 900)).subscribe(_ =>
            AprsService.sendAprsBeacon(config.aprs, true).pipe(
                switchMap(_ => AprsService.sendAprsTelemetry(config.aprs, true))
            ).subscribe(_ => !!config.aprs.waitDtmfInterval ? this.runDtmfDecoder(config) : null)
        );
    }

    private runDtmfDecoder(config: ConfigInterface): void {
        LogService.log('dtmf', 'Start listening');
        let dtmfCode = '';
        let shouldStop = true;

        const decoderSubscription = this.audioDecoder.decode(config.audioDevice, [MultimonModeEnum.DTMF, MultimonModeEnum.TONE], [], ['-T 1750']).pipe(
            skip(1)
        ).subscribe(result => {
            if (shouldStop) {
                LogService.log('dtmf', 'Data decoded', result);

                if (result.type === MultimonModeEnum.TONE) {
                    dtmfCode = '';
                    if (config.voice && config.voice.enable) {
                        shouldStop = false;
                        VoiceService.sendVoice(config.voice.sentence, true).subscribe(_ => shouldStop = true);
                    }
                } else if (result.type === MultimonModeEnum.DTMF) {
                    if (result.data === '#') {
                        LogService.log('dtmf', 'Reset code', dtmfCode);
                        dtmfCode = '';
                    } else {
                        if (config.sstv && config.sstv.enable) {
                            if (!dtmfCode.endsWith(result.data)) {
                                dtmfCode += result.data;
                            }
                            if (dtmfCode === config.sstv.dtmfCode) {
                                dtmfCode = '';
                                shouldStop = false;
                                SstvService.sendImage(config.sstv, true).subscribe(_ => shouldStop = true);
                            }
                        }
                    }
                }
            }
        });

        const interval = 1000 * (config.aprs.waitDtmfInterval ? config.aprs.waitDtmfInterval : 60);
        const timerSubscription = timer(interval, interval).subscribe(_ => {
            if (shouldStop) {
                LogService.log('dtmf', 'Timeout');
                this.audioDecoder.stop().subscribe();
                RadioService.switchOff().subscribe();
                decoderSubscription.unsubscribe();
                timerSubscription.unsubscribe();
            } else {
                LogService.log('dtmf', 'Should timeout, restart', interval);
            }
        });
    }

    private runSensors(config: ConfigInterface): void {
        LogService.log('sensors', 'Started');
        timer(60000, 1000 * (config.sensors.interval ? config.sensors.interval : 60)).subscribe(_ =>
            SensorsService.getAllAndSave(config.sensors).subscribe()
        );
    }

    private runWebcam(config: ConfigInterface): void {
        LogService.log('webcam', 'Started');
        timer(30000, 1000 * (config.webcam.interval ? config.webcam.interval : 30)).subscribe(_ =>
            WebcamService.capture(config.webcam).subscribe()
        );
    }

    private runApi(config: ConfigInterface): void {
        this.api = new ApiService(config);
    }
}
