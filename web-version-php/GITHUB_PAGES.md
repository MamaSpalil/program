# GitHub Pages Deployment Guide

## Enabling the Demo on GitHub Pages

Follow these steps to make the demo publicly accessible:

### 1. Enable GitHub Pages

1. Go to your repository on GitHub: https://github.com/MamaSpalil/program
2. Click on **Settings** tab
3. Scroll down to **Pages** section (in the left sidebar under "Code and automation")
4. Under "Source", select:
   - **Branch**: `claude/demo-website-version-php`
   - **Folder**: `/ (root)`
5. Click **Save**

### 2. Wait for Deployment

GitHub will automatically build and deploy your site. This usually takes 1-3 minutes.

### 3. Access Your Demo

Once deployed, your demo will be available at:

**https://mamaspalil.github.io/program/**

The root index.html will automatically redirect visitors to the demo page.

Direct demo link:
**https://mamaspalil.github.io/program/web-version-php/frontend/demo.html**

### 4. Optional: Add Custom Domain

If you have a custom domain:

1. Add a `CNAME` file with your domain name
2. Configure DNS settings with your domain provider
3. Update the "Custom domain" field in GitHub Pages settings

## Testing Locally

You can also test the demo locally before deploying:

### Option 1: Direct File Opening
```bash
# Navigate to the frontend directory
cd web-version-php/frontend

# Open demo.html in your default browser
# On Linux:
xdg-open demo.html

# On macOS:
open demo.html

# On Windows:
start demo.html
```

### Option 2: Local Web Server
```bash
# Using Python 3
cd web-version-php/frontend
python3 -m http.server 8000

# Then open: http://localhost:8000/demo.html
```

```bash
# Using Node.js (npx http-server)
cd web-version-php/frontend
npx http-server -p 8000

# Then open: http://localhost:8000/demo.html
```

```bash
# Using PHP
cd web-version-php/frontend
php -S localhost:8000

# Then open: http://localhost:8000/demo.html
```

## Demo Features

The standalone demo includes:

- ✅ **Interactive Charts**: Real-time TradingView candlestick charts
- ✅ **Multiple Symbols**: BTC, ETH, BNB, SOL, ADA pairs
- ✅ **Technical Indicators**: RSI, MACD, EMA, ATR calculations
- ✅ **ML Signals**: AI-generated trading signals with confidence levels
- ✅ **Simulated Trading**: Place orders and track positions
- ✅ **Auto-Updates**: Data refreshes every 5 seconds
- ✅ **Responsive**: Works on desktop, tablet, and mobile
- ✅ **No Backend Required**: Pure JavaScript with mock data

## Sharing the Demo

Once deployed on GitHub Pages, you can share the link:

- In your README.md (already added)
- On social media
- In documentation
- With potential users/investors

## Troubleshooting

**Demo not loading?**
- Check that the branch is correct (`claude/demo-website-version-php`)
- Verify the folder is set to `/ (root)`
- Wait a few minutes for GitHub Actions to complete
- Check the Actions tab for any build errors

**Chart not displaying?**
- Ensure you have internet connection (chart library loads from CDN)
- Check browser console for JavaScript errors
- Try a different browser (Chrome, Firefox, Safari, Edge)

**403 Error?**
- Make sure the repository is public, or
- You're logged into GitHub if the repository is private

## Need Help?

For issues or questions, open an issue at:
https://github.com/MamaSpalil/program/issues
