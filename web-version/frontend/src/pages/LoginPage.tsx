import { Box, Typography } from '@mui/material';

export default function LoginPage() {
  return (
    <Box
      sx={{
        display: 'flex',
        flexDirection: 'column',
        alignItems: 'center',
        justifyContent: 'center',
        minHeight: '100vh',
      }}
    >
      <Typography variant="h4">Login</Typography>
      <Typography variant="body2" color="text.secondary">
        Login form to be implemented
      </Typography>
    </Box>
  );
}
