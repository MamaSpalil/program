import { configureStore } from '@reduxjs/toolkit';
import authReducer from './slices/authSlice';
import marketReducer from './slices/marketSlice';
import tradingReducer from './slices/tradingSlice';

export const store = configureStore({
  reducer: {
    auth: authReducer,
    market: marketReducer,
    trading: tradingReducer,
  },
});

export type RootState = ReturnType<typeof store.getState>;
export type AppDispatch = typeof store.dispatch;
