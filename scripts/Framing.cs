using Godot;

public enum ViewFraming
{
	FitToPose,
	ImagePixels
}

public struct FramingTransform
{
	public float Scale;
	public Vector2 Offset;

	public Vector2 Apply(Vector2 p)
	{
		return p * Scale + Offset;
	}
}

// A projected figure only spans about 100-300 px of the 1000x1000 frame.
// That is correct over the photograph but unreadable on a white background,
// so the white view scales the pose up to fill the viewport. This only
// affects what is drawn; recorded coordinates stay in image pixels.
public static class Framing
{
	public static FramingTransform Compute(Vector2[] points, ViewFraming mode, float viewportSize, float padFraction)
	{
		FramingTransform t = new FramingTransform();
		if (mode == ViewFraming.ImagePixels || points.Length == 0)
		{
			t.Scale = 1.0f;
			t.Offset = Vector2.Zero;
			return t;
		}

		float minX = points[0].X;
		float maxX = points[0].X;
		float minY = points[0].Y;
		float maxY = points[0].Y;
		for (int i = 1; i < points.Length; i++)
		{
			minX = Mathf.Min(minX, points[i].X);
			maxX = Mathf.Max(maxX, points[i].X);
			minY = Mathf.Min(minY, points[i].Y);
			maxY = Mathf.Max(maxY, points[i].Y);
		}

		float span = Mathf.Max(maxX - minX, maxY - minY);
		if (span < 0.001f)
		{
			span = 1.0f;
		}
		float padded = span * (1.0f + 2.0f * padFraction);

		t.Scale = viewportSize / padded;
		Vector2 centre = new Vector2((minX + maxX) * 0.5f, (minY + maxY) * 0.5f);
		t.Offset = new Vector2(viewportSize * 0.5f, viewportSize * 0.5f) - centre * t.Scale;
		return t;
	}
}
