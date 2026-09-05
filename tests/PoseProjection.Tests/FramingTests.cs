using Godot;
using Xunit;

public class FramingTests
{
	private static Vector2[] Box()
	{
		return new Vector2[]
		{
			new Vector2(400.0f, 400.0f),
			new Vector2(600.0f, 400.0f),
			new Vector2(400.0f, 500.0f),
			new Vector2(600.0f, 500.0f)
		};
	}

	[Fact]
	public void ImagePixelsFramingIsTheIdentity()
	{
		FramingTransform t = Framing.Compute(Box(), ViewFraming.ImagePixels, 1000.0f, 0.1f);
		Assert.Equal(1.0f, t.Scale, 6);
		Assert.Equal(0.0f, t.Offset.X, 6);
		Assert.Equal(0.0f, t.Offset.Y, 6);
		Assert.Equal(new Vector2(400.0f, 400.0f), t.Apply(new Vector2(400.0f, 400.0f)));
	}

	[Fact]
	public void FitToPoseScalesTheLongerSideToTheViewportLessPadding()
	{
		// bounding box is 200 wide by 100 tall; padded by 10% of 200 on each
		// side gives 240; 1000 / 240 scales it up.
		FramingTransform t = Framing.Compute(Box(), ViewFraming.FitToPose, 1000.0f, 0.1f);
		Assert.Equal(1000.0f / 240.0f, t.Scale, 4);
	}

	[Fact]
	public void FitToPoseCentresTheBoundingBoxInTheViewport()
	{
		FramingTransform t = Framing.Compute(Box(), ViewFraming.FitToPose, 1000.0f, 0.1f);
		Vector2 centre = t.Apply(new Vector2(500.0f, 450.0f));
		Assert.Equal(500.0f, centre.X, 3);
		Assert.Equal(500.0f, centre.Y, 3);
	}

	[Fact]
	public void FitToPoseKeepsEveryPointInsideTheViewport()
	{
		FramingTransform t = Framing.Compute(Box(), ViewFraming.FitToPose, 1000.0f, 0.1f);
		Vector2[] points = Box();
		for (int i = 0; i < points.Length; i++)
		{
			Vector2 p = t.Apply(points[i]);
			Assert.InRange(p.X, 0.0f, 1000.0f);
			Assert.InRange(p.Y, 0.0f, 1000.0f);
		}
	}

	[Fact]
	public void FitToPoseIsUniformSoThePoseIsNotStretched()
	{
		// A wide, flat pose must keep its aspect ratio: the horizontal and
		// vertical gaps scale by the same factor.
		FramingTransform t = Framing.Compute(Box(), ViewFraming.FitToPose, 1000.0f, 0.1f);
		Vector2 a = t.Apply(new Vector2(400.0f, 400.0f));
		Vector2 b = t.Apply(new Vector2(600.0f, 500.0f));
		float sx = (b.X - a.X) / 200.0f;
		float sy = (b.Y - a.Y) / 100.0f;
		Assert.Equal(sx, sy, 5);
	}

	[Fact]
	public void ADegeneratePoseDoesNotDivideByZero()
	{
		Vector2[] single = new Vector2[] { new Vector2(500.0f, 500.0f) };
		FramingTransform t = Framing.Compute(single, ViewFraming.FitToPose, 1000.0f, 0.1f);
		Vector2 p = t.Apply(single[0]);
		Assert.False(float.IsNaN(p.X));
		Assert.False(float.IsNaN(p.Y));
	}
}
