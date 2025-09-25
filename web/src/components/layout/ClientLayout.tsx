"use client";

import { useState } from "react";
import Sidebar from "./Sidebar";
import Topbar from "./Topbar";

export default function ClientLayout({ children }: { children: React.ReactNode }) {
  const [open, setOpen] = useState(false);

  return (
    <div className="h-screen flex">
      {/* Sidebar */}
      <Sidebar open={open} onClose={() => setOpen(false)} />

      {/* Content */}
      <div className="flex flex-col flex-1">
        {/* Topbar (mobile only) */}
        <Topbar onMenuClick={() => setOpen(!open)} />
        {/* Page content */}
        <main className="flex-1 overflow-y-auto p-4 bg-gray-50">
          {children}
        </main>
      </div>
    </div>
  );
}
