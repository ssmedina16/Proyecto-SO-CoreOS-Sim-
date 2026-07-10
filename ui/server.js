import { server } from './server/app.js';

const PORT = process.env.PORT || 5000;
server.listen(PORT, () => {
  console.log(`[CoreOS-Sim] Server listening on http://localhost:${PORT}`);
});
