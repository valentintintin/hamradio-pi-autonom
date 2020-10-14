import { FeatureIntervalInterface } from './config.interface';

export interface SensorsConfigInterface extends FeatureIntervalInterface {
    csvPath: string;
    serialPort?: string;
    serialPortBaudRate?: number;
}
