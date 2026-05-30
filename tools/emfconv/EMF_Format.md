
Chunked format

# File header

``0x89 - EMF 0x0D – 0x0A – 0x1A – 0x0A - 8 Bytes``

## Basic:
```
typedef struct{
  unsigned int chunkLength; // length of chunkData
  unsigned int chunkType; // 4 Ascii bytes
  unsigned char chunkData[]; //Total data
  unsigned int chunkCRC; // Unused for now
}CHUNK;
```

## Header chunk
(required)
```
typedef struct{
  unsigned int chunkLength;
  unsigned int chunkType; //HDR - HDR 0x20
  unsigned int triCount; // Total tris in vertex segment, for now only support tris
  unsgined int modelInfo; // bitfield, unsure, 0 bit is mesh present
  unsigned byte textureFormat; // for included texture data: rgb, rgba, paletted?, (bit indexes) 0 = texture, 1 = vertex color,
//  unsigned byte vertexFormat; //UNUSED
  unsigned int chunkCRC; //Unused but present
} EMF_HEADER;
```

# Vertex chunk
( Multiple Formats, DC and PSP) (pc uses dreamcast for ease of adding features and debugging)

## Dreamcast Vertex Chunk
```
/* gl_fast_vert.h */
glvert_fast_t
 ```
```
typedef struct{
unsigned int chunkLength;
unsigned int chunkType; //DCVT
glvert_fast_t vertices[NUM]; // Ready to Render
unsigned int chunkCRC;
}EMF_DCVT;
```

## PSP Vertex Chunk
```
/* gl_fast_vert.h */
psp_fast_t
 ```
```
typedef struct{
unsigned int chunkLength;
unsigned int chunkType; //PSVT
psp_fast_t vertices[NUM]; // Ready to Render
unsigned int chunkCRC;
}EMF_PSVT;
```

# End Chunk
(required)
```
typedef struct{
unsigned int chunkLength; //0
unsigned int chunkType; //END - END 0x20
unsigned int chunkCRC; //0
}EMF_END;
```