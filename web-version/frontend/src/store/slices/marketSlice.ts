import { createSlice, PayloadAction } from '@reduxjs/toolkit';

interface Candle {
  timestamp: number;
  open: number;
  high: number;
  low: number;
  close: number;
  volume: number;
}

interface MarketState {
  currentPrice: number;
  candles: Candle[];
  symbol: string;
  interval: string;
  loading: boolean;
}

const initialState: MarketState = {
  currentPrice: 0,
  candles: [],
  symbol: 'BTCUSDT',
  interval: '15m',
  loading: false,
};

const marketSlice = createSlice({
  name: 'market',
  initialState,
  reducers: {
    setPrice: (state, action: PayloadAction<number>) => {
      state.currentPrice = action.payload;
    },
    setCandles: (state, action: PayloadAction<Candle[]>) => {
      state.candles = action.payload;
    },
    setSymbol: (state, action: PayloadAction<string>) => {
      state.symbol = action.payload;
    },
    setInterval: (state, action: PayloadAction<string>) => {
      state.interval = action.payload;
    },
    setLoading: (state, action: PayloadAction<boolean>) => {
      state.loading = action.payload;
    },
  },
});

export const { setPrice, setCandles, setSymbol, setInterval, setLoading } = marketSlice.actions;
export default marketSlice.reducer;
