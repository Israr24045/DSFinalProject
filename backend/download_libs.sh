#!/bin/bash

# Download cpp-httplib
echo "Downloading cpp-httplib..."
curl -o backend/httplib.h https://raw.githubusercontent.com/yhirose/cpp-httplib/master/httplib.h

# Download stb_image_write
echo "Downloading stb_image_write..."
curl -o backend/stb_image_write.h https://raw.githubusercontent.com/nothings/stb/master/stb_image_write.h

echo "Libraries downloaded successfully!"
```

Make it executable:
```bash
chmod +x backend/download_libs.sh
./backend/download_libs.sh