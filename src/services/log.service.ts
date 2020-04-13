export class LogService {

    public static LOG_PATH: string = process.cwd();

    public static log(action: string, message: string = null, ...data): void {
        const now = new Date();
        // const fileLog = assetsFolder + 'logs/' + now.getFullYear() + '-' + now.getMonth() + '-' + now.getDate() + '.log';

        const log =
            '[' + now.toLocaleString() + ']' +
            '[' + action.toUpperCase() + ']' +
            (message ? ' --> ' + message : '')
        ;

        if (data && data.length) {
            console.log(log, data);
        } else {
            console.log(log);
        }
        // fs.appendFileSync(fileLog, log + '\n');
    }
}
