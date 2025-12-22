#!/bin/bash
# MP3 Decoder Setup Script
# Bu script minimp3 kütüphanesini otomatik olarak indirir ve kurar

echo "========================================"
echo "MP3 Decoder Kurulum"
echo "========================================"
echo ""

# minimp3.h dosyasının yolunu ayarla
MINIMP3_H="Appli/Core/Inc/minimp3.h"
MINIMP3_C="Appli/Core/Src/minimp3.c"
MINIMP3_URL="https://raw.githubusercontent.com/lieff/minimp3/master/minimp3.h"

echo "1. minimp3.h indiriliyor..."
echo "URL: $MINIMP3_URL"
echo "Hedef: $MINIMP3_H"
echo ""

# wget veya curl ile indir
if command -v wget &> /dev/null; then
    echo "wget kullanılarak indiriliyor..."
    wget -O "$MINIMP3_H" "$MINIMP3_URL"
elif command -v curl &> /dev/null; then
    echo "curl kullanılarak indiriliyor..."
    curl -L -o "$MINIMP3_H" "$MINIMP3_URL"
else
    echo ""
    echo "HATA: wget veya curl bulunamadı!"
    echo "Manuel olarak şu adresten indirin:"
    echo "$MINIMP3_URL"
    echo ""
    exit 1
fi

if [ ! -f "$MINIMP3_H" ]; then
    echo ""
    echo "HATA: minimp3.h indirilemedi!"
    echo "Manuel olarak şu adresten indirin:"
    echo "$MINIMP3_URL"
    echo ""
    exit 1
fi

echo ""
echo "2. minimp3.c güncelleniyor..."

# minimp3.c dosyasını güncelle
cat > "$MINIMP3_C" << 'EOF'
/*
 * minimp3.c - Minimalistic MP3 decoder implementation
 * 
 * Full minimp3 library implementation
 * https://github.com/lieff/minimp3
 */

#define MINIMP3_IMPLEMENTATION
#define MINIMP3_NO_SIMD  /* Disable SIMD for ARM compatibility */

#include "minimp3.h"
EOF

echo ""
echo "========================================"
echo "Kurulum Tamamlandı!"
echo "========================================"
echo ""
echo "MP3 decoder kütüphanesi kullanıma hazır."
echo ""
echo "Sonraki adımlar:"
echo "1. Projenizi STM32CubeIDE'de açın"
echo "2. Projeyi derleyin (Build)"
echo "3. mp3_decoder_example.c dosyasındaki örnekleri inceleyin"
echo "4. MP3_DECODER_README.md dosyasını okuyun"
echo ""
echo "Kullanım örneği:"
echo "  mp3_decoder_handle_t decoder;"
echo "  mp3_decoder_init(&decoder);"
echo "  mp3_decoder_load(&decoder, mp3_data, mp3_size);"
echo "  mp3_decoder_process(&decoder, output_buffer, size, &samples);"
echo ""

