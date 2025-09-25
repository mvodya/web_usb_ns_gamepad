import type { Metadata } from "next";
import "./globals.css";

import ClientLayout from "@/components/layout/ClientLayout";

export const metadata: Metadata = {
  title: "NS Web Gamepad",
  description: "ESP32 gamepad web console",
};

export default function RootLayout({
  children,
}: Readonly<{
  children: React.ReactNode;
}>) {
  return (
    <html lang="en">
      <body>
        <ClientLayout>{children}</ClientLayout>
      </body>
    </html>
  );
}
