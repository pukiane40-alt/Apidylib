import { Switch, Route, Router as WouterRouter, useLocation } from "wouter";
import { QueryClient, QueryClientProvider } from "@tanstack/react-query";
import { Toaster } from "@/components/ui/toaster";
import { TooltipProvider } from "@/components/ui/tooltip";
import { useEffect, useState } from "react";
import { setAuthTokenGetter, useGetMe, getGetMeQueryKey } from "@workspace/api-client-react";

import NotFound from "@/pages/not-found";
import Login from "@/pages/login";
import Dashboard from "@/pages/dashboard";
import Docs from "@/pages/docs";
import Layout from "@/components/layout";

const queryClient = new QueryClient();

// Setup auth token getter for api-client
setAuthTokenGetter(() => {
  return localStorage.getItem("xit_token");
});

function ProtectedRoute({ component: Component, ...rest }: any) {
  const [location, setLocation] = useLocation();
  const { data: user, isLoading, isError } = useGetMe({ 
    query: { 
      queryKey: getGetMeQueryKey(),
      retry: false
    } 
  });

  useEffect(() => {
    if (isError) {
      setLocation("/login");
    }
  }, [isError, setLocation]);

  if (isLoading) {
    return (
      <div className="flex h-screen w-full items-center justify-center bg-background">
        <div className="flex flex-col items-center gap-4">
          <div className="h-8 w-8 animate-spin rounded-full border-4 border-primary border-t-transparent"></div>
          <div className="text-sm font-mono text-muted-foreground animate-pulse">INIT SYSTEM...</div>
        </div>
      </div>
    );
  }

  if (!user) return null;

  return <Component {...rest} />;
}

function AppRouter() {
  const [location, setLocation] = useLocation();
  
  useEffect(() => {
    if (location === "/") {
      const token = localStorage.getItem("xit_token");
      if (token) {
        setLocation("/dashboard");
      } else {
        setLocation("/login");
      }
    }
  }, [location, setLocation]);

  return (
    <Switch>
      <Route path="/login" component={Login} />
      <Route path="/dashboard">
        {() => (
          <Layout>
            <ProtectedRoute component={Dashboard} />
          </Layout>
        )}
      </Route>
      <Route path="/docs">
        {() => (
          <Layout>
            <ProtectedRoute component={Docs} />
          </Layout>
        )}
      </Route>
      <Route component={NotFound} />
    </Switch>
  );
}

function App() {
  useEffect(() => {
    // Force dark mode
    document.documentElement.classList.add("dark");
  }, []);

  return (
    <QueryClientProvider client={queryClient}>
      <TooltipProvider>
        <WouterRouter base={import.meta.env.BASE_URL.replace(/\/$/, "")}>
          <AppRouter />
        </WouterRouter>
        <Toaster />
      </TooltipProvider>
    </QueryClientProvider>
  );
}

export default App;
