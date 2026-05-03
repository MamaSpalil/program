import { useEffect, useRef } from 'react';
import { io, Socket } from 'socket.io-client';

const WS_URL = import.meta.env.VITE_WS_URL || 'ws://localhost:4000';

interface UseWebSocketOptions {
  onConnect?: () => void;
  onDisconnect?: () => void;
  onError?: (error: Error) => void;
}

export const useWebSocket = (options: UseWebSocketOptions = {}) => {
  const socketRef = useRef<Socket | null>(null);

  useEffect(() => {
    const token = localStorage.getItem('accessToken');

    if (!token) {
      return;
    }

    // Create socket connection
    socketRef.current = io(WS_URL, {
      auth: {
        token,
      },
      transports: ['websocket'],
    });

    const socket = socketRef.current;

    socket.on('connect', () => {
      console.log('WebSocket connected');
      options.onConnect?.();
    });

    socket.on('disconnect', () => {
      console.log('WebSocket disconnected');
      options.onDisconnect?.();
    });

    socket.on('connect_error', (error) => {
      console.error('WebSocket connection error:', error);
      options.onError?.(error);
    });

    // Cleanup on unmount
    return () => {
      socket.disconnect();
    };
  }, []);

  const subscribe = (channel: string, callback: (data: any) => void, params?: any) => {
    const socket = socketRef.current;
    if (!socket) return;

    socket.emit('subscribe', { channel, params });
    socket.on(channel, callback);
  };

  const unsubscribe = (channel: string) => {
    const socket = socketRef.current;
    if (!socket) return;

    socket.emit('unsubscribe', { channel });
    socket.off(channel);
  };

  const emit = (event: string, data: any) => {
    socketRef.current?.emit(event, data);
  };

  return {
    subscribe,
    unsubscribe,
    emit,
    socket: socketRef.current,
  };
};
