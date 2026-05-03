import CryptoJS from 'crypto-js';
import config from '../config';

export class Encryption {
  private static key = config.encryptionKey;

  static encrypt(text: string): string {
    return CryptoJS.AES.encrypt(text, this.key).toString();
  }

  static decrypt(ciphertext: string): string {
    const bytes = CryptoJS.AES.decrypt(ciphertext, this.key);
    return bytes.toString(CryptoJS.enc.Utf8);
  }

  static encryptObject(obj: any): string {
    const jsonString = JSON.stringify(obj);
    return this.encrypt(jsonString);
  }

  static decryptObject<T>(ciphertext: string): T {
    const jsonString = this.decrypt(ciphertext);
    return JSON.parse(jsonString);
  }
}
