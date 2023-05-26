import { Entity } from './entity';

export class Sensors extends Entity {
    public createdAt: number = new Date().getTime();
    public voltageBattery: number = null;
    public voltageSolar: number = null;
    public currentBattery: number = null;
    public currentSolar: number = null;
    public currentCharge: number = null;
    public temperatureBattery: number = null;
    public temperatureRtc: number = null;
    public uptime: number = null;
    public light: number = null;
    public pressure: number = null;
    public temperaturePressure: number = null;
    public temperature: number = null;
    public humidity: number = null;
    public rawMpptchg: string;
}
