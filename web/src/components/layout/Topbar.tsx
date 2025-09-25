"use client";

export default function Topbar({ onMenuClick }: { onMenuClick: () => void }) {
  return (
    <header className="md:hidden flex items-center justify-between p-3 border-b bg-white sticky top-0 z-40">
      <button
        className="p-2 border rounded"
        onClick={onMenuClick}
      >
        OP
      </button>
      <div className="font-bold">LOGO</div>
    </header>
  );
}
