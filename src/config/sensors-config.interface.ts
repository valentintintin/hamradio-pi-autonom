import { FeatureIntervalInterface } from './config.interface';

export interface SensorsConfigInterface extends FeatureIntervalInterface {
    serialPort?: string;
    serialPortBaudRate?: number;
}
