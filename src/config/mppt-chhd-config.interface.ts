import { FeatureInterface } from './config.interface';

export interface MpptChhdConfigInterface extends FeatureInterface {
    fake?: boolean;
    watchdog?: boolean;
    shutdownAlert?: boolean;
    shutdownNight?: boolean;
    powerOffVolt?: number;
    powerOnVolt?: number;
    nightLimitVolt?: number;
}
