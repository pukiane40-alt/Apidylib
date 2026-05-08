import { useState } from "react";
import { useLocation } from "wouter";
import { useForm } from "react-hook-form";
import { z } from "zod";
import { zodResolver } from "@hookform/resolvers/zod";
import { useLogin } from "@workspace/api-client-react";
import { TerminalSquare, Lock, User, KeyRound, Loader2, ArrowRight } from "lucide-react";
import { Button } from "@/components/ui/button";
import { Form, FormControl, FormField, FormItem, FormMessage } from "@/components/ui/form";
import { Input } from "@/components/ui/input";
import { toast } from "sonner";
import { useQueryClient } from "@tanstack/react-query";

const loginSchema = z.object({
  username: z.string().min(1, "Username is required"),
  password: z.string().min(1, "Password is required"),
});

type LoginFormValues = z.infer<typeof loginSchema>;

export default function Login() {
  const [, setLocation] = useLocation();
  const loginMutation = useLogin();
  const queryClient = useQueryClient();
  
  const form = useForm<LoginFormValues>({
    resolver: zodResolver(loginSchema),
    defaultValues: {
      username: "",
      password: "",
    },
  });

  const onSubmit = (data: LoginFormValues) => {
    loginMutation.mutate({ data }, {
      onSuccess: (res) => {
        localStorage.setItem("xit_token", res.token);
        queryClient.invalidateQueries();
        setLocation("/dashboard");
        toast.success("Authentication successful", {
          description: `Welcome back, ${res.user.username}.`
        });
      },
      onError: (error) => {
        toast.error("Authentication failed", {
          description: error.data?.error || "Invalid credentials provided."
        });
      }
    });
  };

  return (
    <div className="min-h-screen w-full bg-background flex flex-col items-center justify-center p-4 relative overflow-hidden">
      {/* Decorative background elements */}
      <div className="absolute inset-0 z-0 opacity-20 pointer-events-none">
        <div className="absolute top-1/4 left-1/4 w-96 h-96 bg-primary/20 blur-3xl rounded-full" />
        <div className="absolute bottom-1/4 right-1/4 w-96 h-96 bg-primary/10 blur-3xl rounded-full" />
        <div className="absolute inset-0 bg-[linear-gradient(to_right,#80808012_1px,transparent_1px),linear-gradient(to_bottom,#80808012_1px,transparent_1px)] bg-[size:24px_24px]" />
      </div>

      <div className="w-full max-w-md z-10 space-y-8">
        <div className="flex flex-col items-center text-center space-y-2">
          <div className="h-16 w-16 bg-card border border-border/50 rounded-xl flex items-center justify-center shadow-lg shadow-primary/5 mb-4">
            <TerminalSquare className="h-8 w-8 text-primary" />
          </div>
          <h1 className="text-3xl font-mono font-bold tracking-tighter">XIT-1299</h1>
          <p className="text-muted-foreground text-sm tracking-wide uppercase font-mono">
            SECURE LICENSE MANAGEMENT
          </p>
        </div>

        <div className="bg-card border border-border/50 rounded-xl p-8 shadow-2xl">
          <div className="mb-6 flex items-center text-sm font-mono text-muted-foreground border-b border-border/50 pb-4">
            <Lock className="h-4 w-4 mr-2 text-primary" />
            <span>AUTH_REQUIRED</span>
          </div>

          <Form {...form}>
            <form onSubmit={form.handleSubmit(onSubmit)} className="space-y-5">
              <FormField
                control={form.control}
                name="username"
                render={({ field }) => (
                  <FormItem>
                    <FormControl>
                      <div className="relative">
                        <User className="absolute left-3 top-3 h-4 w-4 text-muted-foreground" />
                        <Input 
                          placeholder="Username" 
                          className="pl-10 bg-background/50 font-mono text-sm border-border/50 h-10" 
                          {...field} 
                        />
                      </div>
                    </FormControl>
                    <FormMessage className="text-xs font-mono text-destructive" />
                  </FormItem>
                )}
              />

              <FormField
                control={form.control}
                name="password"
                render={({ field }) => (
                  <FormItem>
                    <FormControl>
                      <div className="relative">
                        <KeyRound className="absolute left-3 top-3 h-4 w-4 text-muted-foreground" />
                        <Input 
                          type="password"
                          placeholder="••••••••••••" 
                          className="pl-10 bg-background/50 font-mono text-sm border-border/50 h-10" 
                          {...field} 
                        />
                      </div>
                    </FormControl>
                    <FormMessage className="text-xs font-mono text-destructive" />
                  </FormItem>
                )}
              />

              <Button 
                type="submit" 
                className="w-full font-mono font-bold tracking-wide mt-2 group" 
                disabled={loginMutation.isPending}
              >
                {loginMutation.isPending ? (
                  <Loader2 className="h-4 w-4 animate-spin mr-2" />
                ) : (
                  <>
                    AUTHENTICATE
                    <ArrowRight className="h-4 w-4 ml-2 opacity-0 -translate-x-2 group-hover:opacity-100 group-hover:translate-x-0 transition-all duration-200" />
                  </>
                )}
              </Button>
            </form>
          </Form>
        </div>
        
        <div className="text-center">
          <span className="text-xs text-muted-foreground/50 font-mono">
            RESTRICTED SYSTEM ACCESS ONLY
          </span>
        </div>
      </div>
    </div>
  );
}
