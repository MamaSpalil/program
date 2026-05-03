import { Box, Typography } from '@mui/material';

export default function DashboardPage() {
  return (
    <Box sx={{ p: 3 }}>
      <Typography variant="h4" gutterBottom>
        Dashboard
      </Typography>
      <Typography variant="body1" color="text.secondary">
        Welcome to CryptoTrader Web Platform
      </Typography>
    </Box>
  );
}
