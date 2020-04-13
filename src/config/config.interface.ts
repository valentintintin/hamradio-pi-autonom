import { MpptChhdConfigInterface } from './mppt-chhd-config.interface';
import { SstvConfigInterface } from './sstv-config.interface';
import { VoiceConfigInterface } from './voice-config.interface';
import { AprsConfigInterface } from './aprs-config.interface';
import { SensorsConfigInterface } from './sensors-config.interface';
import { WebcamConfigInterface } from './webcam-config.interface';
import { PacketRadioConfigInterface } from './packet-radio-config.interface';

export interface ConfigInterface {
    lat: number;
    lng: number;
    debug?: boolean;
    fakeGpio?: boolean;
    logsPath: string;
    audioDevice?: string;
    packetRadio?: PacketRadioConfigInterface;
    sensors?: SensorsConfigInterface;
    webcam?: WebcamConfigInterface;
    aprs?: AprsConfigInterface;
    sstv?: SstvConfigInterface;
    voice?: VoiceConfigInterface;
    mpptChd?: MpptChhdConfigInterface;
}

export interface FeatureInterface {
    enable: boolean;
}

export interface FeatureIntervalInterface extends FeatureInterface {
    interval: number;
}

