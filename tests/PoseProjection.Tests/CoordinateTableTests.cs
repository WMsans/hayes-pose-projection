using System.IO;
using Godot;
using Xunit;

public class CoordinateTableTests
{
	private static ProjectedFrame MakeFrame(int index, float bias)
	{
		ProjectedFrame f = new ProjectedFrame();
		f.Frame = index;
		f.Points = new Vector2[PoseData.JointCount];
		for (int j = 0; j < f.Points.Length; j++)
		{
			f.Points[j] = new Vector2(100.0f + j + bias, 200.0f + j + bias);
		}
		return f;
	}

	[Fact]
	public void WideTableHasAHeaderAndOneRowPerFrame()
	{
		ProjectedFrame[] frames = new ProjectedFrame[] { MakeFrame(0, 0.0f), MakeFrame(1, 0.0f) };
		StringWriter writer = new StringWriter();
		CoordinateTable.WriteWide(writer, frames);
		string[] lines = writer.ToString().Trim().Split('\n');

		Assert.Equal(3, lines.Length);
		string[] header = lines[0].Trim().Split(',');
		Assert.Equal(29, header.Length);
		Assert.Equal("frame", header[0]);
		Assert.Equal("u0", header[1]);
		Assert.Equal("v0", header[2]);
		Assert.Equal("u13", header[27]);
		Assert.Equal("v13", header[28]);

		string[] row = lines[1].Trim().Split(',');
		Assert.Equal(29, row.Length);
		Assert.Equal("0", row[0]);
	}

	[Fact]
	public void WideTableWritesThreeDecimalsInInvariantCulture()
	{
		ProjectedFrame[] frames = new ProjectedFrame[] { MakeFrame(0, 0.5f) };
		StringWriter writer = new StringWriter();
		CoordinateTable.WriteWide(writer, frames);
		string[] row = writer.ToString().Trim().Split('\n')[1].Trim().Split(',');
		Assert.Equal("100.500", row[1]);
		Assert.Equal("200.500", row[2]);
	}

	[Fact]
	public void LongTableHasOneRowPerJointPerMethod()
	{
		ProjectedFrame[] manual = new ProjectedFrame[] { MakeFrame(0, 0.0f), MakeFrame(1, 0.0f) };
		ProjectedFrame[] godot = new ProjectedFrame[] { MakeFrame(0, 0.0f), MakeFrame(1, 0.0f) };
		string[] names = new string[PoseData.JointCount];
		for (int j = 0; j < names.Length; j++)
		{
			names[j] = "J" + j;
		}

		StringWriter writer = new StringWriter();
		CoordinateTable.WriteLong(writer, manual, godot, names);
		string[] lines = writer.ToString().Trim().Split('\n');

		// header + 2 frames * 14 joints * 2 methods
		Assert.Equal(1 + 2 * 14 * 2, lines.Length);
		Assert.Equal("frame,joint_id,joint_name,method,u,v", lines[0].Trim());
	}

	[Fact]
	public void DiffsAreZeroForIdenticalFrames()
	{
		ProjectedFrame a = MakeFrame(0, 0.0f);
		ProjectedFrame b = MakeFrame(0, 0.0f);
		Assert.Equal(0.0f, CoordinateTable.MaxAbsDiff(a, b), 6);
		Assert.Equal(0.0f, CoordinateTable.MeanAbsDiff(a, b), 6);
	}

	[Fact]
	public void DiffsMeasureTheLargestSingleAxisDeviation()
	{
		ProjectedFrame a = MakeFrame(0, 0.0f);
		ProjectedFrame b = MakeFrame(0, 0.0f);
		b.Points[5] = new Vector2(b.Points[5].X + 0.25f, b.Points[5].Y);
		Assert.Equal(0.25f, CoordinateTable.MaxAbsDiff(a, b), 5);
		// one axis of one joint out of 28 values
		Assert.Equal(0.25f / 28.0f, CoordinateTable.MeanAbsDiff(a, b), 5);
	}

	[Fact]
	public void AgreementTableHasOneRowPerFrame()
	{
		ProjectedFrame[] manual = new ProjectedFrame[] { MakeFrame(0, 0.0f), MakeFrame(1, 0.0f) };
		ProjectedFrame[] godot = new ProjectedFrame[] { MakeFrame(0, 0.0f), MakeFrame(1, 0.0f) };
		StringWriter writer = new StringWriter();
		CoordinateTable.WriteAgreement(writer, manual, godot);
		string[] lines = writer.ToString().Trim().Split('\n');
		Assert.Equal(3, lines.Length);
		Assert.Equal("frame,max_abs_diff_px,mean_abs_diff_px", lines[0].Trim());
	}
}
