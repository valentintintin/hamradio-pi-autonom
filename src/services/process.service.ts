import { ConfigInterface } from '../config/config.interface';
import { LogService } from './log.service';
import { MpptchgService } from './mpptchg.service';
import { AprsService } from './aprs.service';
import { SensorsService } from './sensors.service';
import { timer } from 'rxjs';
import { skip, switchMap } from 'rxjs/operators';
import { VoiceService } from './voice.service';
import { AudioDecoder, MultimonModeEnum } from 'nodejs-arecord-multimon';
import { WebcamService } from './webcam.service';
import { SstvService } from './sstv.service';
import { RadioService } from './radio.service';

export class ProcessService {

    private audioDecoder = new AudioDecoder();

    public run(config: ConfigInterface): void {
        LogService.log('program', 'Start');

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

        this.runDtmfDecoder(config);
    }

    private runMpptChd(config: ConfigInterface): void {
        MpptchgService.battery(config).subscribe();
    }

    private runAprs(config: ConfigInterface): void {
        timer(60, 1000 * (config.aprs.interval ? config.aprs.interval : 900)).subscribe(_ =>
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
            LogService.log('dtmf', 'Should timeout', interval);
            if (shouldStop) {
                LogService.log('dtmf', 'Timeout');
                this.audioDecoder.stop().subscribe();
                RadioService.switchOff().subscribe();
                decoderSubscription.unsubscribe();
                timerSubscription.unsubscribe();
            }
        });
    }

    private runSensors(config: ConfigInterface): void {
        timer(60, 1000 * (config.sensors.interval ? config.sensors.interval : 60)).subscribe(_ =>
            SensorsService.getAllAndSave(config.sensors).subscribe()
        );
    }

    private runWebcam(config: ConfigInterface): void {
        timer(30, 1000 * (config.webcam.interval ? config.webcam.interval : 30)).subscribe(_ =>
            WebcamService.capture(config.webcam).subscribe()
        );
    }
}
