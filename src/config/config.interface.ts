import { MpptChhdConfigInterface } from './mppt-chhd-config.interface';
import { SstvConfigInterface } from './sstv-config.interface';
import { VoiceConfigInterface } from './voice-config.interface';
import { AprsConfigInterface } from './aprs-config.interface';
import { SensorsConfigInterface } from './sensors-config.interface';
import { WebcamConfigInterface } from './webcam-config.interface';
import { PacketRadioConfigInterface } from './packet-radio-config.interface';
import { DashboardConfigInterface } from './dashboard-config.interface';
import { RsyncConfigInterface } from './rsync-config.interface';
import { RepeaterRadioConfigInterface } from './repeater-radio-config.interface';

export interface ConfigInterface {
    callsign: string;
    lat: number;
    lng: number;
    debug?: boolean;
    fakeGpio?: boolean;
    databasePath: string;
    audioDevice?: string;
    packetRadio?: PacketRadioConfigInterface;
    sensors?: SensorsConfigInterface;
    webcam?: WebcamConfigInterface;
    aprs?: AprsConfigInterface;
    sstv?: SstvConfigInterface;
    voice?: VoiceConfigInterface;
    mpptChd?: MpptChhdConfigInterface;
    dashboard?: DashboardConfigInterface;
    rsync?: RsyncConfigInterface;
    repeater?: RepeaterRadioConfigInterface
}

export interface FeatureInterface {
    enable: boolean;
}

export interface FeatureIntervalInterface extends FeatureInterface {
    interval: number;
}

