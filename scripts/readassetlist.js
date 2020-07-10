const fs = require('fs');

let crewlist = JSON.parse(fs.readFileSync(`${__dirname}/../../website/static/structured/crew.json`));

let assets = {};
crewlist.forEach(crew => {
if (crew.max_rarity > 3) {
	assets[crew.symbol] = `https://assets.datacore.app/${crew.imageUrlFullBody}`;
    }
})

fs.writeFileSync(`${__dirname}/../data/assets.json`, JSON.stringify({assets}));
