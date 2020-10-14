import { ProcessService } from './services/process.service';
import { CommandMpptChd, CommunicationMpptchdService } from './services/communication-mpptchd.service';
import { delay, skip, switchMap } from 'rxjs/operators';
import { GpioEnum, GpioService } from './services/gpio.service';
import { WebcamService } from './services/webcam.service';
import { ConfigInterface } from './config/config.interface';
import { VoiceService } from './services/voice.service';
import { AprsService } from './services/aprs.service';
import { SstvService } from './services/sstv.service';
import { AudioDecoder, MultimonModeEnum } from 'nodejs-arecord-multimon';
import { SensorsService } from './services/sensors.service';
import { RadioService } from './services/radio.service';
import { TncService } from './services/tnc.service';
import { DatabaseService } from './services/database.service';
import { EnumVariable } from './models/variables';
import { DashboardService } from './services/dashboard.service';
import { ToneService } from './services/tone.service';
import { exec } from 'child_process';
import { MpptchgService } from './services/mpptchg.service';
import { RsyncService } from './services/rsync.service';
import { LogService } from './services/log.service';
import { SerialService } from './services/serial.service';

export const assetsFolder: string = process.cwd() + '/assets';

require('date.format');

const config = loadConfig();

ProcessService.debug = !!config.debug;

GpioService.USE_FAKE = !!config.fakeGpio;

if (config.mpptChd) {
    CommunicationMpptchdService.USE_FAKE = !!config.mpptChd.fake;
    CommunicationMpptchdService.DEBUG = !!config.mpptChd.debugI2C;
}

if (config.webcam) {
    WebcamService.USE_FAKE = !!config.webcam.fake;
}

if (config.sensors) {
    SerialService.USE_FAKE = !!config.fakeGpio;
}

switch (process.argv[process.argv.length - 1]) {
    case 'dtmf':
        RadioService.switchOn().subscribe(_ => {
            new AudioDecoder().decode(config.audioDevice,
                [MultimonModeEnum.DTMF, MultimonModeEnum.TONE, MultimonModeEnum.AFSK1200, MultimonModeEnum.AFSK2400],
                [], ['-T 1750']).pipe(
                skip(1)
            ).subscribe(result => {
                console.log(result);
            });
        });
        break;

    case 'repeat':
        RadioService.listenAndRepeat(config.repeater.seconds).subscribe();
        break;

    case 'tones':
        ToneService.send1750(true, true, 5).pipe(
            delay(2500),
            switchMap(_ => ToneService.sendOk(true, true)),
            delay(2500),
            switchMap(_ => ToneService.sendError()),
        ).subscribe();
        break;

    case 'voice':
        VoiceService.sendVoice(config.voice.sentence, false, config.voice).subscribe();
        break;

    case 'direwolf-connection':
        TncService.instance.connectTnc().subscribe();
        break;

    case 'direwolf-listen':
        TncService.instance.packetsRecevied.subscribe(p => console.log(p));
        break;

    case 'aprs-beacon':
        AprsService.sendAprsBeacon(config.aprs).subscribe();
        break;

    case 'aprs-telem':
        AprsService.sendAprsTelemetry(config.aprs, config.sensors).subscribe();
        break;

    case 'db-reset-seqtelem':
        DatabaseService.updateVariable(EnumVariable.SEQ_TELEMETRY, 0).subscribe();
        break;

    case 'sstv':
        SstvService.sendImage(config.sstv).subscribe();
        break;

    case 'webcam':
        WebcamService.capture(config.webcam).subscribe();
        break;

    case 'sensors':
        SensorsService.getAllCurrent().subscribe();
        break;

    case 'gpio-on':
        GpioService.set(GpioEnum.RelayRadio, true).pipe(
            switchMap(_ => GpioService.set(GpioEnum.PTT, true)),
            switchMap(_ => GpioService.set(GpioEnum.RelayBorneWifi, true)),
        ).subscribe();
        break;

    case 'gpio-off':
        GpioService.set(GpioEnum.RelayRadio, false).pipe(
            switchMap(_ => GpioService.set(GpioEnum.PTT, false)),
            switchMap(_ => GpioService.set(GpioEnum.RelayBorneWifi, false)),
        ).subscribe();
        break;

    case 'mppt-status':
        setInterval(_ => CommunicationMpptchdService.instance.getStatus().subscribe(d => console.log(d)), 1500);
        break;

    case 'mppt-stop-wd':
        CommunicationMpptchdService.instance.disableWatchdog().pipe(
            switchMap(_ => CommunicationMpptchdService.instance.getStatus())
        ).subscribe(d => console.log(d));
        break;

    case 'shutdown':
        const restartDate = new Date();
        restartDate.setMinutes(restartDate.getMinutes() + 5);
        MpptchgService.shutdownAndWakeUpAtDate(restartDate, 30).subscribe(wakeupDate => {
            setInterval(_ => CommunicationMpptchdService.instance.getStatus().subscribe(d => console.log(wakeupDate, d)), 1500);
            setTimeout(_ => {
                exec('halt');
            }, 30000);
        });
        break;

    case 'mppt-stop-wd-loop':
        CommunicationMpptchdService.instance.send(CommandMpptChd.PWROFFV, 11000).subscribe(_ => {
            setInterval(_ => {
                CommunicationMpptchdService.instance.disableWatchdog().pipe(
                    switchMap(_ => CommunicationMpptchdService.instance.getStatus())
                ).subscribe(d => {
                    if (d.alertAsserted) {
                        setTimeout(_ => {
                            exec('halt');
                        }, 30000);
                        console.log('Alert !!', d);
                    } else {
                        console.log(d.values.batteryVoltage);
                    }
                });
            }, 1500);
        });
        break;

    case 'mppt-start-wd-loop':
        CommunicationMpptchdService.instance.send(CommandMpptChd.PWROFFV, 11000).subscribe(_ => {
            setInterval(_ => {
                CommunicationMpptchdService.instance.enableWatchdog(10, 180).pipe(
                    switchMap(_ => CommunicationMpptchdService.instance.getStatus())
                ).subscribe(d => {
                    if (d.alertAsserted) {
                        setTimeout(_ => {
                            exec('halt');
                        }, 30000);
                        console.log('Alert !!', d);
                    } else {
                        console.log(d.values.batteryVoltage);
                    }
                });
            }, 1500);
        });
        break;

    case 'dashboard':
        DatabaseService.openDatabase(config.databasePath).subscribe(_ => new DashboardService(config));
        break;

    case 'rsync':
        ProcessService.debug = false;
        RsyncService.runSync(config).subscribe();
        break;

    case 'remove-old-logs':
        DatabaseService.openDatabase(config.databasePath).subscribe(_ => LogService.removeTooOld());
        break;

    case 'program':
        new ProcessService().run(config);
        break;

    default:
        console.log('Liste des commandes :');
        console.table({
            'dtmf': 'Listen to DTMF code or 1750 Hz tone',
            'tones': 'Send 1750, OK and error',
            'repeat': 'Listen 10s the frequency and replay it over the air',
            'voice': 'Do the voice command',
            'direwolf-connection': 'Test connection to Direwolf',
            'direwolf-listen': 'Listen for AX25 packets',
            'aprs-beacon': 'Send APRS beacon',
            'aprs-telem': 'Send APRS telemetry',
            'db-reset-seqtelem': 'Reset sequence current APRS telemetry packet',
            'sstv': 'Send the last photo take over the air in Robot36',
            'webcam': 'Capture one photo',
            'sensors': 'Read all sensors values',
            'gpio-on': 'Set all GPIO used as HIGH',
            'gpio-off': 'Set all GPIO used as LOW',
            'mppt-status': 'Get the status of the MPPTChg board',
            'mppt-stop-wd': 'Stop the watchdog of the MPPTChg board',
            'mppt-stop-wd-loop': 'Stop the watchdog of the MPPTChg board in loop',
            'mppt-start-wd-loop': 'Start the watchdog of the MPPTChg board in loop',
            'shutdown': 'Shutdown the PI in 30 seconds and restart it 2 minutes later',
            'dashboard': 'Run only the dashboard Web interface',
            'rsync': 'Send test file to remote server',
            'remove-old-logs': 'Remove old logs',
            'program': 'Run the program',
        });
        break;
}

function loadConfig(): ConfigInterface {
    let configFile = require(process.argv[process.argv.length - 2]);
    if (!configFile || !configFile.config) {
        configFile = require(process.argv[process.argv.length - 1]);
        if (!configFile || !configFile.config) {
            throw new Error('No config file !');
        }
    }
    return configFile.config;
}
