const fs = require('fs');
const path = require('path');

const root = path.resolve(__dirname, '..');
const exts = new Set(['.c', '.h']);

function walk(dir) {
    const entries = fs.readdirSync(dir, { withFileTypes: true });
    for (const e of entries) {
        const full = path.join(dir, e.name);
        if (e.isDirectory()) {
            if (e.name === 'Debug' || e.name === 'Release' || e.name === 'build') continue;
            walk(full);
        } else if (e.isFile() && exts.has(path.extname(e.name).toLowerCase())) {
            const src = fs.readFileSync(full, 'utf8');
            if (!src.includes('\t')) continue;
            const dst = src.replace(/\t/g, '    ');
            fs.writeFileSync(full, dst, 'utf8');
            console.log(full);
        }
    }
}

walk(root);
console.log('Done.');
