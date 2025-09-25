"use client";

import Link from "next/link";
import { usePathname } from "next/navigation";

const links = [
  { href: "/", label: "Gamepad" },
  { href: "/printer", label: "Splat Printer" },
  { href: "/scripter", label: "Scripter" },
  { href: "/api", label: "API" },
];

export default function Sidebar({ open, onClose }: { open: boolean; onClose: () => void }) {
  const pathname = usePathname();

  return (
    <>
      {/* Overlay (mobile only) */}
      {open && (
        <div
          className="fixed inset-0 bg-black/40 z-20 md:hidden"
          onClick={onClose}
        />
      )}

      <aside
        className={`
          fixed md:static left-0 h-full w-64 bg-white border-r p-4 z-30
          transform transition-transform duration-200 ease-in-out
          ${open ? "translate-x-0" : "-translate-x-full"}
          md:translate-x-0
          md:top-0 top-[56px]  /* mobile offset */
        `}>
        <div className="text-xl font-bold mb-4 hidden md:block">LOGO</div>
        <nav className="flex flex-col gap-2">
          {links.map((link) => (
            <Link
              key={link.href}
              href={link.href}
              className={`px-3 py-2 rounded ${pathname === link.href
                ? "bg-blue-100 font-semibold"
                : "hover:bg-gray-100"
                }`}
              onClick={onClose}>
              {link.label}
            </Link>
          ))}
        </nav>

        <div className="absolute bottom-2 left-0 w-full px-3 py-2 text-sm border-t">
          Frontend status: ✅ Connected
        </div>
      </aside>
    </>
  );
}
