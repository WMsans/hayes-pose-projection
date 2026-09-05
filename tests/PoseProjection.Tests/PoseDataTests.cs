using System.IO;
using Godot;
using Xunit;

public class PoseDataTests
{
	public static PoseData Load()
	{
		string dir = DataFilesTests.DataDir();
		StreamReader poses = new StreamReader(Path.Combine(dir, "poses.txt"));
		StreamReader names = new StreamReader(Path.Combine(dir, "joint-names.txt"));
		string focal = File.ReadAllText(Path.Combine(dir, "focal.txt"));
		PoseData data = PoseData.Parse(poses, names, focal);
		poses.Dispose();
		names.Dispose();
		return data;
	}

	[Fact]
	public void ReadsFocalAndCounts()
	{
		PoseData data = Load();
		Assert.Equal(1148.6f, data.Focal, 3);
		Assert.Equal(20, data.Frames.Length);
		Assert.Equal(14, data.JointNames.Length);
	}

	[Fact]
	public void ReadsJointNamesWithoutQuotes()
	{
		PoseData data = Load();
		Assert.Equal("Hip", data.JointNames[0]);
		Assert.Equal("RAnkle", data.JointNames[3]);
		Assert.Equal("Neck", data.JointNames[7]);
		Assert.Equal("RWrist", data.JointNames[13]);
	}

	[Fact]
	public void ReadsFirstFrameCameraAndJoints()
	{
		PoseData data = Load();
		PoseFrame frame = data.Frames[0];
		Assert.Equal(1761.27853428116f, frame.CameraWorld.X, 2);
		Assert.Equal(-5078.00659454077f, frame.CameraWorld.Y, 2);
		Assert.Equal(1606.2649598335f, frame.CameraWorld.Z, 2);

		Assert.Equal(14, frame.JointsWorld.Length);
		Assert.Equal(9.433470f, frame.JointsWorld[0].X, 3);
		Assert.Equal(-15.833000f, frame.JointsWorld[0].Y, 3);
		Assert.Equal(158.005997f, frame.JointsWorld[0].Z, 3);

		Assert.Equal(16.278038f, frame.JointsWorld[7].X, 3);
		Assert.Equal(-65.465576f, frame.JointsWorld[7].Y, 3);
		Assert.Equal(630.855408f, frame.JointsWorld[7].Z, 3);
	}

	[Fact]
	public void ConvertsToGodotSpaceInMetres()
	{
		PoseData data = Load();
		PoseFrame frame = data.Frames[0];
		// (x, z, -y) / 1000
		Assert.Equal(0.00943347f, frame.JointsGodot[0].X, 6);
		Assert.Equal(0.15800600f, frame.JointsGodot[0].Y, 6);
		Assert.Equal(0.01583300f, frame.JointsGodot[0].Z, 6);

		Assert.Equal(1.76127853f, frame.CameraGodot.X, 6);
		Assert.Equal(1.60626496f, frame.CameraGodot.Y, 6);
		Assert.Equal(5.07800659f, frame.CameraGodot.Z, 6);
	}

	[Fact]
	public void ConversionPreservesHandedness()
	{
		Vector3 x = PoseFrame.ToGodot(new Vector3(1000, 0, 0));
		Vector3 y = PoseFrame.ToGodot(new Vector3(0, 1000, 0));
		Vector3 z = PoseFrame.ToGodot(new Vector3(0, 0, 1000));
		// determinant of the mapped basis must be +1, not -1
		float det = x.Dot(y.Cross(z));
		Assert.Equal(1.0f, det, 5);
	}

	[Fact]
	public void ComputesTheCentroidOfTheFourteenJoints()
	{
		PoseData data = Load();
		PoseFrame frame = data.Frames[0];
		Assert.Equal(30.35914f, frame.CentroidWorld.X, 3);
		Assert.Equal(-110.13630f, frame.CentroidWorld.Y, 3);
		Assert.Equal(269.79306f, frame.CentroidWorld.Z, 3);
	}

	[Fact]
	public void EveryFrameHasFourteenJoints()
	{
		PoseData data = Load();
		for (int i = 0; i < data.Frames.Length; i++)
		{
			Assert.Equal(14, data.Frames[i].JointsWorld.Length);
			Assert.Equal(14, data.Frames[i].JointsGodot.Length);
		}
	}
}
