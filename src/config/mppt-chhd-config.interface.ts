import { FeatureInterface } from './config.interface';

export interface MpptChhdConfigInterface extends FeatureInterface {
    fake?: boolean;
    debugI2C?: boolean;
    watchdog?: boolean;
    batteryLowAlert?: boolean;
    nightAlert?: boolean;
    powerOffVolt?: number;
    powerOnVolt?: number;
    pauseBeforePowerOffReachedVolt?: number;
    pauseBeforePowerOffReachedSeconds?: number;
    nightLimitVolt?: number;
    nightSleepTimeSeconds?: number;
    nightRunSleepTimeSeconds?: number;
}
