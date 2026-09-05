using System;
using System.IO;
using Xunit;

public class DataFilesTests
{
	public static string DataDir()
	{
		string dir = AppContext.BaseDirectory;
		for (int i = 0; i < 8; i++)
		{
			string candidate = Path.Combine(dir, "data");
			if (Directory.Exists(candidate))
			{
				return candidate;
			}
			dir = Path.GetFullPath(Path.Combine(dir, ".."));
		}
		throw new DirectoryNotFoundException("could not locate the data directory");
	}

	[Fact]
	public void PosesFileHasTwentyRowsOfFortyFiveColumns()
	{
		string[] lines = File.ReadAllLines(Path.Combine(DataDir(), "poses.txt"));
		Assert.Equal(20, lines.Length);
		for (int i = 0; i < lines.Length; i++)
		{
			string[] parts = lines[i].Split('\t', StringSplitOptions.RemoveEmptyEntries);
			Assert.Equal(45, parts.Length);
		}
	}

	[Fact]
	public void FocalFileHoldsTheExpectedValue()
	{
		string text = File.ReadAllText(Path.Combine(DataDir(), "focal.txt")).Trim();
		Assert.Equal("1148.6", text);
	}

	[Fact]
	public void ThereAreTwentyFrameImages()
	{
		string[] files = Directory.GetFiles(Path.Combine(DataDir(), "frames"), "*.png");
		Assert.Equal(20, files.Length);
	}
}
