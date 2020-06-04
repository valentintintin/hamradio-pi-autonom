import { FeatureInterface } from './config.interface';

export class DashboardConfigInterface implements FeatureInterface {
    enable: boolean;
    port?: number;
    apikey?: string;
    mail?: string;
}
