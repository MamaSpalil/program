import { Box, Typography } from '@mui/material';

export default function RegisterPage() {
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
      <Typography variant="h4">Register</Typography>
      <Typography variant="body2" color="text.secondary">
        Registration form to be implemented
      </Typography>
    </Box>
  );
}
