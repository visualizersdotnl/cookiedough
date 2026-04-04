
// cookiedough -- face & vertex

#if !defined(CKD_RETRO3D_PRIMITIVES)
#define CKD_RETRO3D_PRIMITIVES

namespace retro3D
{
    struct Face 
    {
        unsigned iA;
        unsigned iB;
        unsigned iC;
    };

    struct Vertex
    {
        Vector3 position;
        uint32_t ARGB;
    };
}

#endif // CKD_RETRO3D_PRIMITIVES
