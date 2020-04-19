import { GpioService } from './services/gpio.service';
import { CommunicationMpptchdService } from './services/communication-mpptchd.service';
import { LogService } from './services/log.service';
import { config } from '../assets/config';
import { ProcessService } from './services/process.service';
import { WebcamService } from './services/webcam.service';

export const assetsFolder: string = process.cwd() + '/assets';

require('date.format');

if (!config) {
    throw new Error('No config file !');
}

export let debug: boolean = !!config.debug;

LogService.LOG_PATH = config.logsPath;
GpioService.USE_FAKE = !!config.fakeGpio;

if (config.mpptChd) {
    CommunicationMpptchdService.USE_FAKE = !!config.mpptChd.fake;
    CommunicationMpptchdService.DEBUG = !!config.mpptChd.debugI2C;
}

if (config.webcam) {
    WebcamService.USE_FAKE = !!config.webcam.fake;
}

new ProcessService().run(config);

// todo Add Direwolf Service with option for baudrate and why not add beacon and telemetry ?
