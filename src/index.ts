import { ProcessService } from './services/process.service';
import { CommunicationMpptchdService } from './services/communication-mpptchd.service';
import { skip, switchMap } from 'rxjs/operators';
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

switch (process.argv[process.argv.length - 1]) {
    case 'dtmf':
        new AudioDecoder().decode(config.audioDevice,
            [MultimonModeEnum.DTMF, MultimonModeEnum.TONE, MultimonModeEnum.AFSK1200, MultimonModeEnum.AFSK2400],
            [], ['-T 1750']).pipe(
            skip(1)
        ).subscribe(result => {
            console.log(result);
        });
        break;

    case 'repeat':
        RadioService.listenAndRepeat(config.repeater.seconds).subscribe();
        break;

    case 'voice':
        VoiceService.sendVoice(config.voice.sentence).subscribe();
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

    case 'dashboard':
        DatabaseService.openDatabase(config.databasePath).subscribe(_ => new DashboardService(config));
        break;

    case 'program':
    default:
        new ProcessService().run(config);
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
