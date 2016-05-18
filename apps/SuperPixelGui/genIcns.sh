mkdir superpi.iconset
sips -z 16 16     superpi.png --out superpi.iconset/icon_16x16.png
sips -z 32 32     superpi.png --out superpi.iconset/icon_16x16@2x.png
sips -z 32 32     superpi.png --out superpi.iconset/icon_32x32.png
sips -z 64 64     superpi.png --out superpi.iconset/icon_32x32@2x.png
sips -z 128 128   superpi.png --out superpi.iconset/icon_128x128.png
sips -z 256 256   superpi.png --out superpi.iconset/icon_128x128@2x.png
sips -z 256 256   superpi.png --out superpi.iconset/icon_256x256.png
sips -z 512 512   superpi.png --out superpi.iconset/icon_256x256@2x.png
sips -z 512 512   superpi.png --out superpi.iconset/icon_512x512.png
sips -z 1024 1024   superpi.png --out superpi.iconset/icon_512x512.png
iconutil -c icns superpi.iconset
rm -r superpi.iconset
