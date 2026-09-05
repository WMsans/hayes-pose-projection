using Godot;
using Xunit;

public class ManualProjectorTests
{
	// Ground truth from the independent Python implementation, frame 00.
	private static readonly float[,] Frame00 = new float[,]
	{
		{ 502.333f, 517.539f }, { 480.252f, 503.825f }, { 485.110f, 497.054f },
		{ 428.339f, 577.699f }, { 524.751f, 531.463f }, { 500.437f, 541.094f },
		{ 462.049f, 553.164f }, { 500.299f, 422.890f }, { 530.248f, 433.410f },
		{ 563.850f, 478.121f }, { 581.759f, 527.989f }, { 478.020f, 434.501f },
		{ 483.533f, 470.911f }, { 475.622f, 516.824f }
	};

	// Ground truth for frame 19, whose camera is on the opposite side of the room.
	private static readonly float[,] Frame19 = new float[,]
	{
		{ 501.860f, 508.145f }, { 527.319f, 506.708f }, { 533.577f, 557.377f },
		{ 510.035f, 543.297f }, { 476.692f, 509.565f }, { 499.567f, 554.163f },
		{ 479.916f, 539.331f }, { 498.953f, 412.108f }, { 475.023f, 426.704f },
		{ 448.962f, 474.435f }, { 434.662f, 520.478f }, { 523.354f, 428.259f },
		{ 540.535f, 480.548f }, { 552.195f, 530.374f }
	};

	[Fact]
	public void ProjectsFrameZeroToTheKnownPixelCoordinates()
	{
		AssertFrameMatches(0, Frame00);
	}

	[Fact]
	public void ProjectsFrameNineteenToTheKnownPixelCoordinates()
	{
		AssertFrameMatches(19, Frame19);
	}

	[Fact]
	public void EveryJointOfEveryFrameLandsInsideTheImage()
	{
		PoseData data = PoseDataTests.Load();
		ManualProjector projector = new ManualProjector();
		for (int f = 0; f < data.Frames.Length; f++)
		{
			projector.Begin(data.Frames[f], data.Focal);
			for (int j = 0; j < PoseData.JointCount; j++)
			{
				Vector2 p = projector.Project(j);
				Assert.InRange(p.X, 0.0f, 1000.0f);
				Assert.InRange(p.Y, 0.0f, 1000.0f);
			}
		}
	}

	[Fact]
	public void TheNeckProjectsAboveTheAnklesInEveryFrame()
	{
		// v grows downward, so a correct orientation puts the neck at a
		// smaller v than both ankles. This catches an inverted up vector.
		PoseData data = PoseDataTests.Load();
		ManualProjector projector = new ManualProjector();
		for (int f = 0; f < data.Frames.Length; f++)
		{
			projector.Begin(data.Frames[f], data.Focal);
			float neck = projector.Project(7).Y;
			Assert.True(neck < projector.Project(3).Y, "frame " + f + " right ankle");
			Assert.True(neck < projector.Project(6).Y, "frame " + f + " left ankle");
		}
	}

	private static void AssertFrameMatches(int frameIndex, float[,] expected)
	{
		PoseData data = PoseDataTests.Load();
		ManualProjector projector = new ManualProjector();
		projector.Begin(data.Frames[frameIndex], data.Focal);
		for (int j = 0; j < PoseData.JointCount; j++)
		{
			Vector2 p = projector.Project(j);
			Assert.Equal(expected[j, 0], p.X, 2);
			Assert.Equal(expected[j, 1], p.Y, 2);
		}
	}
}
