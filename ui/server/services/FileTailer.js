import fs from 'fs';
import path from 'path';

/**
 * FileTailer — Monitors a file for new content appended after the initial offset.
 * Used to tail CSV log files written by the C++ simulation processes.
 */
export class FileTailer {
  constructor(filePath, onLine) {
    this.filePath = filePath;
    this.onLine = onLine;
    this.offset = 0;
    this.watcher = null;
    this.dirWatcher = null;
  }

  start() {
    this.offset = 0;
    try {
      const stats = fs.statSync(this.filePath);
      this.offset = stats.size;
    } catch {
      this.offset = 0;
    }

    if (!fs.existsSync(this.filePath)) {
      this._watchParentDir();
    } else {
      this._watchFile();
    }
  }

  _watchParentDir() {
    const parentDir = path.dirname(this.filePath);
    const baseName = path.basename(this.filePath);
    this.dirWatcher = fs.watch(parentDir, (eventType, filename) => {
      if (filename === baseName && fs.existsSync(this.filePath)) {
        this.dirWatcher.close();
        this.dirWatcher = null;
        this._watchFile();
        this._readNewContent();
      }
    });
  }

  _watchFile() {
    this.watcher = fs.watch(this.filePath, (eventType) => {
      if (eventType === 'change') {
        this._readNewContent();
      }
    });
  }

  _readNewContent() {
    try {
      if (!fs.existsSync(this.filePath)) return;
      const stats = fs.statSync(this.filePath);
      if (stats.size < this.offset) this.offset = 0;
      if (stats.size === this.offset) return;

      const fd = fs.openSync(this.filePath, 'r');
      const buffer = Buffer.alloc(stats.size - this.offset);
      fs.readSync(fd, buffer, 0, buffer.length, this.offset);
      fs.closeSync(fd);

      this.offset = stats.size;
      const lines = buffer.toString('utf8').split('\n');
      for (const raw of lines) {
        const line = raw.trim();
        if (line && !line.startsWith('timestamp_ms')) {
          this.onLine(line);
        }
      }
    } catch (err) {
      console.error(`[FileTailer] Error reading ${this.filePath}:`, err.message);
    }
  }

  stop() {
    if (this.watcher) { this.watcher.close(); this.watcher = null; }
    if (this.dirWatcher) { this.dirWatcher.close(); this.dirWatcher = null; }
  }
}
