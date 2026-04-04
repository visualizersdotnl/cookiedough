
// cookiedough -- face & vertex

#if !defined(CKD_RETRO3D_PRIMITIVES)
#define CKD_RETRO3D_PRIMITIVES

namespace retro3D
{
	// triangle indices
	struct Face 
	{
		unsigned iA;
		unsigned iB;
		unsigned iC;
	};

	// std. textured polygon vertex
	struct Vertex
	{
		Vector3 position;
		uint32_t ARGB;
		Vector2 UV;
	};

	// triangle filler vertex (post projection)
	struct Projected
	{
		int iX, iY;
		float projZ;
		uint32_t ARGB;
		Vector2 UV;
	};
}

#endif // CKD_RETRO3D_PRIMITIVES
 