import { Link, useLocation } from "wouter";
import { KeyRound, FileCode2, LogOut, TerminalSquare, Activity } from "lucide-react";
import { useLogout, useGetMe, getGetMeQueryKey, useHealthCheck, getHealthCheckQueryKey } from "@workspace/api-client-react";
import { useQueryClient } from "@tanstack/react-query";
import { Button } from "./ui/button";

export default function Layout({ children }: { children: React.ReactNode }) {
  const [location, setLocation] = useLocation();
  const queryClient = useQueryClient();
  const logout = useLogout();
  
  const { data: user } = useGetMe({ 
    query: { queryKey: getGetMeQueryKey() } 
  });

  const { data: health } = useHealthCheck({
    query: { queryKey: getHealthCheckQueryKey(), refetchInterval: 30000 }
  });

  const handleLogout = () => {
    logout.mutate(undefined, {
      onSuccess: () => {
        localStorage.removeItem("xit_token");
        queryClient.clear();
        setLocation("/login");
      }
    });
  };

  return (
    <div className="min-h-screen flex w-full bg-background selection:bg-primary/30 selection:text-primary">
      {/* Sidebar */}
      <aside className="w-64 flex-shrink-0 border-r border-border/50 bg-card flex flex-col hidden md:flex">
        <div className="h-16 flex items-center px-6 border-b border-border/50">
          <TerminalSquare className="w-5 h-5 text-primary mr-3" />
          <span className="font-mono font-bold tracking-tight text-lg">@xit1299</span>
        </div>
        
        <nav className="flex-1 py-6 px-4 space-y-2 overflow-y-auto">
          <Link href="/dashboard" className={`flex items-center px-3 py-2.5 rounded-sm text-sm font-medium transition-colors ${location === '/dashboard' ? 'bg-primary/10 text-primary border border-primary/20' : 'text-muted-foreground hover:text-foreground hover:bg-secondary'}`}>
            <KeyRound className="w-4 h-4 mr-3" />
            Keys
          </Link>
          <Link href="/docs" className={`flex items-center px-3 py-2.5 rounded-sm text-sm font-medium transition-colors ${location === '/docs' ? 'bg-primary/10 text-primary border border-primary/20' : 'text-muted-foreground hover:text-foreground hover:bg-secondary'}`}>
            <FileCode2 className="w-4 h-4 mr-3" />
            API Docs
          </Link>
        </nav>
        
        <div className="p-4 border-t border-border/50 space-y-4">
          <div className="flex items-center justify-between text-xs text-muted-foreground font-mono">
            <span className="flex items-center">
              <Activity className={`w-3.5 h-3.5 mr-2 ${health?.status === 'ok' ? 'text-green-500' : 'text-yellow-500'}`} />
              SYS: {health?.status === 'ok' ? 'ONLINE' : 'DEGRADED'}
            </span>
          </div>
          <div className="flex items-center justify-between pt-2 border-t border-border/50">
            <div className="flex flex-col">
              <span className="text-xs text-muted-foreground">Logged in as</span>
              <span className="text-sm font-medium font-mono">{user?.username}</span>
            </div>
            <Button variant="ghost" size="icon" onClick={handleLogout} className="h-8 w-8 text-muted-foreground hover:text-destructive hover:bg-destructive/10">
              <LogOut className="w-4 h-4" />
            </Button>
          </div>
        </div>
      </aside>

      {/* Main Content */}
      <main className="flex-1 flex flex-col min-w-0 overflow-hidden">
        {/* Mobile Header */}
        <header className="h-14 flex items-center justify-between px-4 border-b border-border/50 bg-card md:hidden">
          <div className="flex items-center">
            <TerminalSquare className="w-5 h-5 text-primary mr-2" />
            <span className="font-mono font-bold tracking-tight">@xit1299</span>
          </div>
          <div className="flex items-center space-x-2">
            <Link href="/dashboard">
              <Button variant={location === '/dashboard' ? 'secondary' : 'ghost'} size="sm" className="h-8 px-2 text-xs">Keys</Button>
            </Link>
            <Link href="/docs">
              <Button variant={location === '/docs' ? 'secondary' : 'ghost'} size="sm" className="h-8 px-2 text-xs">Docs</Button>
            </Link>
            <Button variant="ghost" size="icon" onClick={handleLogout} className="h-8 w-8 text-muted-foreground">
              <LogOut className="w-4 h-4" />
            </Button>
          </div>
        </header>

        <div className="flex-1 overflow-auto p-4 md:p-8">
          <div className="max-w-6xl mx-auto">
            {children}
          </div>
        </div>
      </main>
    </div>
  );
}
