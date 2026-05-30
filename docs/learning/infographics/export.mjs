/**
 * Export HTML infographics to PNG
 *
 * Usage:
 *   node export.mjs                  # Export all blocks
 *   node export.mjs --file 02        # Export specific file (02-system-architecture.html)
 */

import { chromium } from 'playwright';
import express from 'express';
import { fileURLToPath } from 'url';
import { dirname, join, basename } from 'path';
import { readdirSync, mkdirSync, existsSync } from 'fs';

const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);
const OUTPUT_DIR = join(__dirname, 'output');
const PORT = 3000;

async function main() {
  const args = process.argv.slice(2);
  const fileFilter = args.includes('--file') ? args[args.indexOf('--file') + 1] : null;

  // 创建输出目录
  if (!existsSync(OUTPUT_DIR)) mkdirSync(OUTPUT_DIR, { recursive: true });

  // 启动本地服务器
  const app = express();
  app.use(express.static(__dirname));
  const server = app.listen(PORT);

  // 启动浏览器
  const browser = await chromium.launch({ headless: true });

  try {
    const htmlFiles = readdirSync(__dirname)
      .filter(f => f.endsWith('.html') && (!fileFilter || f.startsWith(fileFilter)));

    console.log(`Processing ${htmlFiles.length} HTML file(s)\n`);

    for (const file of htmlFiles) {
      await exportFile(browser, file);
    }

    console.log(`\nDone! Output: ${OUTPUT_DIR}`);
  } finally {
    await browser.close();
    server.close();
  }
}

async function exportFile(browser, filename) {
  console.log(`Processing: ${filename}`);

  const page = await browser.newPage();
  await page.goto(`http://localhost:${PORT}/${filename}`, { waitUntil: 'networkidle' });

  // 等待字体加载
  await page.waitForFunction('document.fonts.ready');

  // 获取所有 visual-block ID
  const blockIDs = await page.evaluate(() =>
    Array.from(document.querySelectorAll('[data-export="block"]')).map(el => el.id)
  );

  if (blockIDs.length === 0) {
    console.log('  No blocks found');
    await page.close();
    return;
  }

  for (const id of blockIDs) {
    console.log(`  Exporting: ${id}.png`);

    // 临时移除 margin
    await page.evaluate((blockId) => {
      const el = document.getElementById(blockId);
      el.dataset.originalMargin = el.style.margin;
      el.style.margin = '0';
    }, id);

    // 截图
    const block = await page.locator(`#${id}`);
    await block.screenshot({ path: join(OUTPUT_DIR, `${id}.png`) });

    // 恢复 margin
    await page.evaluate((blockId) => {
      const el = document.getElementById(blockId);
      el.style.margin = el.dataset.originalMargin || '';
    }, id);
  }

  await page.close();
}

main().catch(err => {
  console.error('Error:', err);
  process.exit(1);
});
