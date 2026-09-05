using Godot;

// Draws the projected skeleton. In ImagePixels mode the coordinates are
// used exactly as computed, which is what makes the photograph overlay
// line up. FitToPose only rescales what is drawn.
public partial class ProjectionOverlay : Control
{
	private const float BoneWidth = 4.0f;
	private const float JointRadius = 6.0f;

	public Vector2[] Points;
	public ViewFraming Mode = ViewFraming.FitToPose;
	public bool ShowPhotograph;
	public Texture2D Photograph;

	public void Refresh(Vector2[] points)
	{
		Points = points;
		QueueRedraw();
	}

	public override void _Draw()
	{
		Vector2 size = new Vector2(Main.ImageSize, Main.ImageSize);
		if (ShowPhotograph && Photograph != null)
		{
			DrawTextureRect(Photograph, new Rect2(Vector2.Zero, size), false);
		}
		else
		{
			DrawRect(new Rect2(Vector2.Zero, size), Colors.White, true);
		}

		if (Points == null)
		{
			return;
		}

		ViewFraming mode = Mode;
		if (ShowPhotograph)
		{
			// Any rescaling would break registration with the photograph.
			mode = ViewFraming.ImagePixels;
		}
		FramingTransform t = Framing.Compute(Points, mode, Main.ImageSize, 0.1f);

		for (int b = 0; b < Bones.All.Length; b++)
		{
			Vector2 a = t.Apply(Points[Bones.All[b].A]);
			Vector2 c = t.Apply(Points[Bones.All[b].B]);
			DrawLine(a, c, Bones.ColorFor(Bones.All[b].Side), BoneWidth, true);
		}

		for (int j = 0; j < Points.Length; j++)
		{
			DrawCircle(t.Apply(Points[j]), JointRadius, new Color(0.10f, 0.10f, 0.10f));
		}
	}
}
