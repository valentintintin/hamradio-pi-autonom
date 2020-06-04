import { ConfigInterface } from '../config/config.interface';
import { LogService } from './log.service';
import { EventMpptChg, MpptchgService } from './mpptchg.service';
import { AprsService } from './aprs.service';
import { SensorsService } from './sensors.service';
import { of, Subscription, timer } from 'rxjs';
import { catchError, map, skip, switchMap, tap } from 'rxjs/operators';
import { VoiceService } from './voice.service';
import { AudioDecoder, MultimonModeEnum } from 'nodejs-arecord-multimon';
import { WebcamService } from './webcam.service';
import { SstvService } from './sstv.service';
import { RadioService } from './radio.service';
import { DatabaseService } from './database.service';
import { DashboardService } from './dashboard.service';
import { exec } from 'child_process';
import { CommunicationMpptchdService } from './communication-mpptchd.service';

export class ProcessService {

    public static debug: boolean;

    private audioDecoder = new AudioDecoder();
    private mpptchgSubscription: Subscription;
    private api: DashboardService;
    private dtmfDecoderShouldStop: boolean;
    private stopDirectly: boolean;

    public run(config: ConfigInterface): void {
        LogService.log('program', 'Starting');

        try {
            DatabaseService.openDatabase(config.logsPath).pipe(
                switchMap(_ => RadioService.pttOff(true))
            ).subscribe(_ => {
                this.testFunction(config);

                if (config.mpptChd && config.mpptChd.enable) {
                    this.runMpptChd(config);
                }

                if (config.aprs && config.aprs.enable) {
                    this.runAprs(config);
                    if (!!config.aprs.waitDtmfInterval) {
                        this.runDtmfDecoder(config);
                    }
                }

                if (config.sensors && config.sensors.enable) {
                    this.runSensors(config);
                }

                if (config.webcam && config.webcam.enable) {
                    this.runWebcam(config);
                }

                if (config.dashboard && config.dashboard.enable) {
                    this.runApi(config);
                }

                if (config.sftp && config.sftp.enable) {
                    this.runSftp(config);
                }

                process.on('exit', event => this.exitHandler(config, event));
                process.on('SIGINT', event => this.exitHandler(config, event));
                process.on('SIGTERM', event => this.exitHandler(config, event));
                process.on('SIGQUIT', event => this.exitHandler(config, event));

                LogService.log('program', 'Started');
            });
        } catch (e) {
            this.exitHandler(config, 0);
        }
    }

    private exitHandler(config: ConfigInterface, event) {
        let stop = of(null);

        if (!this.stopDirectly) {
            LogService.log('program', 'Stopping', event);

            if (this.mpptchgSubscription) {
                this.mpptchgSubscription.unsubscribe();

                if (event !== EventMpptChg.ALERT) {
                    stop = stop.pipe(
                        switchMap(_ =>
                            MpptchgService.stopWatchdog().pipe(
                                tap(_ => LogService.log('mpptChd', 'Watchdog disabled if enabled')),
                                catchError(e => {
                                    LogService.log('mpptChd', 'Watchdog impossible to disabled (if enabled) !');
                                    return of(null);
                                })
                            )
                        )
                    );
                }
            }

            stop = stop.pipe(
                catchError(err => {
                    LogService.log('program', 'Stopped KO', err);
                    return of(null);
                })
            );
        }

        stop.subscribe(_ => {
            if (this.stopDirectly) {
                LogService.log('program', 'Stopped', event);
            }
            this.stopDirectly = true;
            if (!ProcessService.debug) {
                exec('halt');
            }
            process.exit(0);
        });
    }

    private runMpptChd(config: ConfigInterface): void {
        this.mpptchgSubscription = MpptchgService.startBatteryManager(config).subscribe();
        MpptchgService.events.on(EventMpptChg.ALERT, _ => this.exitHandler(config, EventMpptChg.ALERT));
    }

    private runAprs(config: ConfigInterface): void {
        LogService.log('aprs', 'Started');
        timer(1000 * (config.aprs.interval ?? 900), 1000 * (config.aprs.interval ?? 900)).subscribe(_ =>
            AprsService.sendAprsBeacon(config.aprs, true).pipe(
                switchMap(_ => AprsService.sendAprsTelemetry(config.aprs, config.sensors, true))
            ).subscribe(_ => {
                if (!AprsService.alreadyInUse) {
                    this.dtmfDecoderShouldStop = true;
                    const interval = 1000 * (config.aprs.waitDtmfInterval ?? 60);
                    const timerSubscription = timer(interval, interval).subscribe(_ => {
                        if (this.dtmfDecoderShouldStop) {
                            LogService.log('dtmf', 'Timeout');
                            this.audioDecoder.stop().subscribe();
                            RadioService.switchOff().subscribe();
                            timerSubscription.unsubscribe();
                        } else {
                            LogService.log('dtmf', 'Should timeout, restart', interval);
                        }
                    });
                }
            })
        );
    }

    private runDtmfDecoder(config: ConfigInterface): void {
        LogService.log('dtmf', 'Start listening');
        let dtmfCode = '';

        this.audioDecoder.decode(config.audioDevice, [MultimonModeEnum.DTMF, MultimonModeEnum.TONE], [], ['-T 1750']).pipe(
            skip(1)
        ).subscribe(result => {
            LogService.log('dtmf', 'Data decoded', result);

            if (result.type === MultimonModeEnum.TONE) {
                dtmfCode = '';
                if (config.voice && config.voice.enable) {
                    this.dtmfDecoderShouldStop = false;
                    CommunicationMpptchdService.instance.getStatus().pipe(
                        map(data => {
                            return config.voice.sentence
                                .replace('batteryVoltage', data.values.batteryVoltage / 1000 + '')
                                .replace('chargeCurrent', data.values.chargeCurrent + '')
                                ;
                        }),
                        switchMap(sentence => VoiceService.sendVoice(sentence, true)),
                        switchMap(_ => RadioService.listenAndRepeat(config.repeater.seconds, false))
                    ).subscribe(_ => this.dtmfDecoderShouldStop = true);
                }
            } else if (result.type === MultimonModeEnum.DTMF) {
                if (result.data === '#') {
                    LogService.log('dtmf', 'Reset code', dtmfCode);
                    dtmfCode = '';
                } else if (result.data === '*') {
                    this.dtmfDecoderShouldStop = false;
                    if (config.sstv && config.sstv.enable && dtmfCode === config.sstv.dtmfCode) {
                        SstvService.sendImage(config.sstv, true).subscribe(_ => this.dtmfDecoderShouldStop = true);
                    } else if (dtmfCode === config.packetRadio.dtmfCode) {
                        RadioService.keepOn = !RadioService.keepOn;
                        LogService.log('radio', 'State keepOn set by DTMF', RadioService.keepOn);
                        VoiceService.sendVoice('Radio ' + (RadioService.keepOn ? 'allumée' : 'éteinte'), true).subscribe(_ => this.dtmfDecoderShouldStop = true);
                    } else {
                        LogService.log('dtmf', 'Code not recognized', dtmfCode);
                        VoiceService.sendVoice('Erreur code, ' + dtmfCode, true).subscribe(_ => this.dtmfDecoderShouldStop = true);
                    }
                    dtmfCode = '';
                } else if (!dtmfCode.endsWith(result.data)) {
                    dtmfCode += result.data;
                }
            }
        });
    }

    private runSensors(config: ConfigInterface): void {
        LogService.log('sensors', 'Started');
        timer(1000 * (config.sensors.interval ?? 60), 1000 * (config.sensors.interval ?? 60)).subscribe(_ =>
            SensorsService.getAllCurrentAndSave(config.sensors).subscribe()
        );
    }

    private runWebcam(config: ConfigInterface): void {
        LogService.log('webcam', 'Started');
        timer(1000 * (config.webcam.interval ?? 30), 1000 * (config.webcam.interval ?? 30)).subscribe(_ =>
            WebcamService.captureAndSend(config.webcam, config.sftp).subscribe()
        );
    }

    private runApi(config: ConfigInterface): void {
        this.api = new DashboardService(config);
    }

    private runSftp(config: ConfigInterface): void {
        LogService.log('sftp', 'Started');
        timer(60000, 1000 * (config.sftp.interval ?? 60)).subscribe(_ => {
            LogService.send(config.sftp, config.logsPath).subscribe();
        });
    }

    private testFunction(config: ConfigInterface): void {
    }
}
