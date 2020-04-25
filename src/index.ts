import { ProcessService } from './services/process.service';

export const assetsFolder: string = process.cwd() + '/assets';

require('date.format');

import(process.argv[process.argv.length - 1]).then(config => {
    if (!config || !config.config) {
        throw new Error('No config file !');
    }

    new ProcessService().run(config.config);
});
