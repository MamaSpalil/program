import { createSlice, PayloadAction } from '@reduxjs/toolkit';

interface Order {
  id: string;
  symbol: string;
  side: string;
  type: string;
  quantity: number;
  price?: number;
  status: string;
}

interface Position {
  id: string;
  symbol: string;
  side: string;
  quantity: number;
  entryPrice: number;
  unrealizedPnl: number;
}

interface TradingState {
  orders: Order[];
  positions: Position[];
  loading: boolean;
}

const initialState: TradingState = {
  orders: [],
  positions: [],
  loading: false,
};

const tradingSlice = createSlice({
  name: 'trading',
  initialState,
  reducers: {
    setOrders: (state, action: PayloadAction<Order[]>) => {
      state.orders = action.payload;
    },
    addOrder: (state, action: PayloadAction<Order>) => {
      state.orders.unshift(action.payload);
    },
    updateOrder: (state, action: PayloadAction<Order>) => {
      const index = state.orders.findIndex((o) => o.id === action.payload.id);
      if (index !== -1) {
        state.orders[index] = action.payload;
      }
    },
    setPositions: (state, action: PayloadAction<Position[]>) => {
      state.positions = action.payload;
    },
    updatePosition: (state, action: PayloadAction<Position>) => {
      const index = state.positions.findIndex((p) => p.id === action.payload.id);
      if (index !== -1) {
        state.positions[index] = action.payload;
      } else {
        state.positions.push(action.payload);
      }
    },
    setLoading: (state, action: PayloadAction<boolean>) => {
      state.loading = action.payload;
    },
  },
});

export const { setOrders, addOrder, updateOrder, setPositions, updatePosition, setLoading } =
  tradingSlice.actions;
export default tradingSlice.reducer;
