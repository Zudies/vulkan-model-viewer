namespace ModelViewer
{
    public static class Version
    {
        public static string String
        {
            get
            {
                return string.Format("{0}.{1}.{2}", Major, Minor, Revision);
            }
        }

        public static int Major = 0;
        public static int Minor = 1;
        public static int Revision = 0;
    }
}
