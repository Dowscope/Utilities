using System;
using System.Security.Cryptography;

namespace BellSchedulerPasswordGenerator
{
    class Program
    {
        private const int SaltSize = 16;
        private const int HashSize = 20;
        private const int HashIterations = 1000;

        static void Main(string[] args)
        {
            Console.Write("Password: ");
            string password = Console.ReadLine();

            Console.WriteLine();
            Console.WriteLine("Hash:");
            Console.WriteLine(Convert.ToBase64String(HashPassword(password)));
        }

        private static byte[] HashPassword(string password)
        {
            byte[] salt = new byte[SaltSize];
            RandomNumberGenerator.Fill(salt);
            return Hash(password, salt);
        }

        private static byte[] Hash(string password, byte[] salt)
        {
            using var pbkdf2 = new Rfc2898DeriveBytes(
                password,
                salt,
                HashIterations,
                HashAlgorithmName.SHA1);

            byte[] ret = new byte[SaltSize + HashSize];

            Array.Copy(salt, 0, ret, 0, SaltSize);
            Array.Copy(pbkdf2.GetBytes(HashSize), 0, ret, SaltSize, HashSize);

            return ret;
        }
    }
}
