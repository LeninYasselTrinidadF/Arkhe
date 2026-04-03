---
description: Envía el archivo actual a Arkhe actualizando el archivo arkhe_bridge.json
---
# Send to Arkhe (Antigravity Extension)

Este workflow simula el comando "Send to Arkhe" para Antigravity. Cuando el usuario invoque esta función (por ejemplo, con `/send-to-arkhe` o "envía esto a Arkhe"), debes seguir estos pasos escrupulosamente:

1. **Identificar el archivo objetivo:**
   Si el usuario no especifica un archivo en su mensaje, lee la ruta de "Active Document" en tu bloque `<ADDITIONAL_METADATA>`.

// turbo
2. **Ejecutar el conector automático:**
   Abre una terminal y ejecuta el script (pasando la ruta ABSOLUTA del archivo resuelto entre comillas):
   ```bash
   node c:/Users/USUARIO/source/repos/Arkhe/.agent/scripts/send_arkhe.js "<RUTA_ABSOLUTA>"
   ```

3. **Confirmación:**
   Notifica al usuario que acabas de enviar el comando y confírmale el nombre del archivo.
