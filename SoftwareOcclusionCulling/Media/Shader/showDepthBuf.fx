float4 VSMain(uint vert : SV_VertexID) : SV_Position
{
		float x = (vert & 1) * 4.0f - 1.0f;
		float y = (vert >> 1) * 4.0f - 1.0f;
		return float4(x, y, 0.5f, 1.0f);
}

Texture2D tex : register(t0);

float4 PSMain(float4 pos : SV_Position) : SV_Target
{
		int inX = (int) pos.x;
		int inY = (int) pos.y;

		// perform swizzle
		int x = (inX & 1) + ((inY & 1) << 1) + ((inX & 2) << 1) + ((inY & 2) << 2) + ((inX & ~3) << 2);
		int y = inY >> 2;

		return tex.Load(int3(x, y, 0));
}