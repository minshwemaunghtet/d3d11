struct vs_out 
{
    float4 position : SV_POSITION; // required output of VS
};

// constant buffer
cbuffer ConstantBuffer : register(b0)
{
    float x_offset;
    float y_offset;
};

vs_out vs_main(float2 pos : Position) 
{
  vs_out output; 
  output.position = float4(pos.x + x_offset, pos.y + y_offset, 0.0f, 1.0f);  //for the movement

  return output;
}

float4 ps_main(vs_out input) : SV_TARGET 
{
  return float4( 1.0, 0.0, 1.0, 1.0 ); // return an RGBA colour
}